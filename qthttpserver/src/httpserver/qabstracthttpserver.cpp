// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtHttpServer/qabstracthttpserver.h>

#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qhttpserverresponder.h>
#include <private/qabstracthttpserver_p.h>
#include <private/qhttpserverrequest_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetaobject.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>

#if QT_CONFIG(ssl)
#include <QtNetwork/qsslserver.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcHttpServer, "qt.httpserver")

/*!
    \internal
*/
QAbstractHttpServerPrivate::QAbstractHttpServerPrivate()
{
}

/*!
    \internal
*/
void QAbstractHttpServerPrivate::handleNewConnections()
{
    Q_Q(QAbstractHttpServer);
    auto tcpServer = qobject_cast<QTcpServer *>(q->sender());
    Q_ASSERT(tcpServer);
    while (auto socket = tcpServer->nextPendingConnection()) {
        auto request = new QHttpServerRequest(socket->peerAddress());  // TODO own tcp server could pre-allocate it
        QObject::connect(socket, &QTcpSocket::readyRead, q,
                         [this, request, socket] () {
            handleReadyRead(socket, request);
        });

        QObject::connect(socket, &QTcpSocket::disconnected, socket, [request, socket] () {
            if (!request->d->handling)
                socket->deleteLater();
        });

        QObject::connect(socket, &QObject::destroyed, socket, [request] () {
            delete request;
        });
    }
}

/*!
    \internal
*/
void QAbstractHttpServerPrivate::handleReadyRead(QTcpSocket *socket,
                                                 QHttpServerRequest *request)
{
    Q_Q(QAbstractHttpServer);
    Q_ASSERT(socket);
    Q_ASSERT(request);

    if (!socket->isTransactionStarted())
        socket->startTransaction();

    if (!request->d->parse(socket)) {
        socket->disconnectFromHost();
        return;
    }

    if (request->d->state != QHttpServerRequestPrivate::State::AllDone)
        return; // Partial read

    qCDebug(lcHttpServer) << "Request:" << *request;

#if defined(QT_WEBSOCKETS_LIB)
    if (request->d->upgrade) { // Upgrade
        const auto &upgradeValue = request->value(QByteArrayLiteral("upgrade"));
        if (upgradeValue.compare(QByteArrayLiteral("websocket"), Qt::CaseInsensitive) == 0) {
            static const auto signal = QMetaMethod::fromSignal(
                        &QAbstractHttpServer::newWebSocketConnection);
            if (q->handleRequest(*request, socket) && q->isSignalConnected(signal)) {
                // Socket will now be managed by websocketServer
                socket->disconnect();
                socket->rollbackTransaction();
                // And we can delete the request, as it's no longer used
                delete request;
                websocketServer.handleConnection(socket);
                Q_EMIT socket->readyRead();
            } else {
                qWarning(lcHttpServer, "WebSocket received but no slots connected to "
                                       "QWebSocketServer::newConnection or request not handled");
                q->missingHandler(*request, socket);
                socket->disconnectFromHost();
            }
            return;
        }
    }
#endif

    socket->commitTransaction();
    request->d->handling = true;
    if (!q->handleRequest(*request, socket))
        q->missingHandler(*request, socket);
    request->d->handling = false;
    if (socket->state() == QAbstractSocket::UnconnectedState)
        socket->deleteLater();
    else if (socket->bytesAvailable() > 0)
        QMetaObject::invokeMethod(socket, &QAbstractSocket::readyRead, Qt::QueuedConnection);
}

/*!
    \class QAbstractHttpServer
    \since 6.4
    \inmodule QtHttpServer
    \brief API to subclass to implement an HTTP server.

    Subclass this class and override \c handleRequest() to create an HTTP
    server. Use \c listen() or \c bind() to start listening to incoming
    connections.
*/

/*!
    Creates an instance of QAbstractHttpServer with the parent \a parent.
*/
QAbstractHttpServer::QAbstractHttpServer(QObject *parent)
    : QAbstractHttpServer(*new QAbstractHttpServerPrivate, parent)
{}

/*!
    \internal
*/
QAbstractHttpServer::~QAbstractHttpServer()
    = default;

/*!
    \internal
*/
QAbstractHttpServer::QAbstractHttpServer(QAbstractHttpServerPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
#if defined(QT_WEBSOCKETS_LIB)
    Q_D(QAbstractHttpServer);
    connect(&d->websocketServer, &QWebSocketServer::newConnection,
            this, &QAbstractHttpServer::newWebSocketConnection);
#endif
}

/*!
    Tries to bind a \c QTcpServer to \a address and \a port.

    Returns the server port upon success, 0 otherwise.
*/
quint16 QAbstractHttpServer::listen(const QHostAddress &address, quint16 port)
{
#if QT_CONFIG(ssl)
    Q_D(QAbstractHttpServer);
    QTcpServer *tcpServer;
    if (d->sslEnabled) {
        auto sslServer = new QSslServer(this);
        sslServer->setSslConfiguration(d->sslConfiguration);
        tcpServer = sslServer;
    } else {
        tcpServer = new QTcpServer(this);
    }
#else
    auto tcpServer = new QTcpServer(this);
#endif
    const auto listening = tcpServer->listen(address, port);
    if (listening) {
        bind(tcpServer);
        return tcpServer->serverPort();
    } else {
        qCCritical(lcHttpServer, "listen failed: %ls",
                   qUtf16Printable(tcpServer->errorString()));
    }

    delete tcpServer;
    return 0;
}

/*!
    Returns the list of ports this instance of QAbstractHttpServer
    is listening to.

    This function has the same guarantee as QObject::children,
    the latest server added is the last entry in the vector.

    \sa servers()
*/
QList<quint16> QAbstractHttpServer::serverPorts()
{
    QList<quint16> ports;
    auto children = findChildren<QTcpServer *>();
    ports.reserve(children.count());
    std::transform(children.cbegin(), children.cend(), std::back_inserter(ports),
                   [](const QTcpServer *server) { return server->serverPort(); });
    return ports;
}

/*!
    Bind the HTTP server to given TCP \a server over which
    the transmission happens. It is possible to call this function
    multiple times with different instances of TCP \a server to
    handle multiple connections and ports, for example both SSL and
    non-encrypted connections.

    After calling this function, every _new_ connection will be
    handled and forwarded by the HTTP server.

    It is the user's responsibility to call QTcpServer::listen() on
    the \a server.

    If the \a server is null, then a new, default-constructed TCP
    server will be constructed, which will be listening on a random
    port and all interfaces.

    The \a server will be parented to this HTTP server.

    \sa QTcpServer, QTcpServer::listen()
*/
void QAbstractHttpServer::bind(QTcpServer *server)
{
    Q_D(QAbstractHttpServer);
    if (!server) {
        server = new QTcpServer(this);
        if (!server->listen()) {
            qCCritical(lcHttpServer, "QTcpServer listen failed (%ls)",
                       qUtf16Printable(server->errorString()));
        }
    } else {
        if (!server->isListening())
            qCWarning(lcHttpServer) << "The TCP server" << server << "is not listening.";
        server->setParent(this);
    }
    QObjectPrivate::connect(server, &QTcpServer::pendingConnectionAvailable, d,
                            &QAbstractHttpServerPrivate::handleNewConnections,
                            Qt::UniqueConnection);
}

/*!
    Returns list of child TCP servers of this HTTP server.

    \sa serverPorts()
 */
QList<QTcpServer *> QAbstractHttpServer::servers() const
{
    return findChildren<QTcpServer *>().toVector();
}

#if defined(QT_WEBSOCKETS_LIB)
/*!
    \fn QAbstractHttpServer::newWebSocketConnection()
    This signal is emitted every time a new WebSocket connection is
    available.

    \sa hasPendingWebSocketConnections(), nextPendingWebSocketConnection()
*/

/*!
    Returns \c true if the server has pending WebSocket connections;
    otherwise returns \c false.

    \sa newWebSocketConnection(), nextPendingWebSocketConnection()
*/
bool QAbstractHttpServer::hasPendingWebSocketConnections() const
{
    Q_D(const QAbstractHttpServer);
    return d->websocketServer.hasPendingConnections();
}

/*!
    Returns the next pending connection as a connected QWebSocket
    object. \nullptr is returned if this function is called when
    there are no pending connections.

    \note The returned QWebSocket object cannot be used from another
    thread.

    \sa newWebSocketConnection(), hasPendingWebSocketConnections()
*/
std::unique_ptr<QWebSocket> QAbstractHttpServer::nextPendingWebSocketConnection()
{
    Q_D(QAbstractHttpServer);
    return std::unique_ptr<QWebSocket>(d->websocketServer.nextPendingConnection());
}
#endif

/*!
    \internal
*/
QHttpServerResponder QAbstractHttpServer::makeResponder(const QHttpServerRequest &request,
                                                        QTcpSocket *socket)
{
    return QHttpServerResponder(request, socket);
}

/*!
    \fn QAbstractHttpServer::handleRequest(const QHttpServerRequest &request,
                                           QTcpSocket *socket)
    Overload this function to handle each incoming \a request from \a socket,
    by examining the \a request and sending the appropriate response back to
    \a socket. Returns \c true if the \a request was handled. Otherwise,
    returns \c false. If returning \c false, \c missingHandler() will be
    called afterwards.
*/

/*!
    \fn QAbstractHttpServer::missingHandler(const QHttpServerRequest &request, QTcpSocket *socket)

    This function is called whenever \c handleRequest() returns \c false.
    The \a request and \a socket parameters are the same as \c handleRequest()
    was called with.
*/

#if QT_CONFIG(ssl)
/*!
    Turns the server into an HTTPS server.

    The next listen() call will use the given \a certificate, \a privateKey,
    and \a protocol.
*/
void QAbstractHttpServer::sslSetup(const QSslCertificate &certificate,
                                   const QSslKey &privateKey,
                                   QSsl::SslProtocol protocol)
{
    QSslConfiguration conf;
    conf.setLocalCertificate(certificate);
    conf.setPrivateKey(privateKey);
    conf.setProtocol(protocol);
    sslSetup(conf);
}

/*!
    Turns the server into an HTTPS server.

    The next listen() call will use the given \a sslConfiguration.
*/
void QAbstractHttpServer::sslSetup(const QSslConfiguration &sslConfiguration)
{
    Q_D(QAbstractHttpServer);
    d->sslConfiguration = sslConfiguration;
    d->sslEnabled = true;
}
#endif

QT_END_NAMESPACE

#include "moc_qabstracthttpserver.cpp"
