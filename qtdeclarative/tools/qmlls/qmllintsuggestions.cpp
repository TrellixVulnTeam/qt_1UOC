// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "qmllintsuggestions.h"
#include <QtLanguageServer/private/qlanguageserverspec_p.h>
#include <QtQmlCompiler/private/qqmljslinter_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>

using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lintLog, "qt.languageserver.lint")

QT_BEGIN_NAMESPACE
namespace QmlLsp {

static DiagnosticSeverity severityFromMsgType(QtMsgType t)
{
    switch (t) {
    case QtDebugMsg:
        return DiagnosticSeverity::Hint;
    case QtInfoMsg:
        return DiagnosticSeverity::Information;
    case QtWarningMsg:
        return DiagnosticSeverity::Warning;
    case QtCriticalMsg:
    case QtFatalMsg:
        break;
    }
    return DiagnosticSeverity::Error;
}

static void codeActionHandler(
        const QByteArray &, const CodeActionParams &params,
        LSPPartialResponse<std::variant<QList<std::variant<Command, CodeAction>>, std::nullptr_t>,
                           QList<std::variant<Command, CodeAction>>> &&response)
{
    QList<std::variant<Command, CodeAction>> responseData;

    for (const Diagnostic &diagnostic : params.context.diagnostics) {
        if (!diagnostic.data.has_value())
            continue;

        const auto &data = diagnostic.data.value();

        int version = data[u"version"].toInt();
        QJsonArray suggestions = data[u"suggestions"].toArray();

        QList<TextDocumentEdit> edits;
        QString message;
        for (const QJsonValue &suggestion : suggestions) {
            QString replacement = suggestion[u"replacement"].toString();
            message += suggestion[u"message"].toString() + u"\n";

            TextEdit textEdit;
            textEdit.range = { Position { suggestion[u"lspBeginLine"].toInt(),
                                          suggestion[u"lspBeginCharacter"].toInt() },
                               Position { suggestion[u"lspEndLine"].toInt(),
                                          suggestion[u"lspEndCharacter"].toInt() } };
            textEdit.newText = replacement.toUtf8();

            TextDocumentEdit textDocEdit;
            textDocEdit.textDocument = { params.textDocument, version };
            textDocEdit.edits.append(textEdit);

            edits.append(textDocEdit);
        }
        message.chop(1);
        WorkspaceEdit edit;
        edit.documentChanges = edits;

        CodeAction action;
        action.kind = u"refactor.rewrite"_s.toUtf8();
        action.edit = edit;
        action.title = message.toUtf8();

        responseData.append(action);
    }

    response.sendResponse(responseData);
}

void QmlLintSuggestions::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerCodeActionRequestHandler(&codeActionHandler);
}

void QmlLintSuggestions::setupCapabilities(const QLspSpecification::InitializeParams &,
                                           QLspSpecification::InitializeResult &serverInfo)
{
    serverInfo.capabilities.codeActionProvider = true;
}

QmlLintSuggestions::QmlLintSuggestions(QLanguageServer *server, QmlLsp::QQmlCodeModel *codeModel)
    : m_server(server), m_codeModel(codeModel)
{
    QObject::connect(m_codeModel, &QmlLsp::QQmlCodeModel::updatedSnapshot, this,
                     &QmlLintSuggestions::diagnose, Qt::DirectConnection);
}

void QmlLintSuggestions::diagnose(const QByteArray &url)
{
    const int maxInvalidMsec = 4000;
    qCDebug(lintLog) << "diagnose start";
    QmlLsp::OpenDocumentSnapshot snapshot = m_codeModel->snapshotByUrl(url);
    QList<Diagnostic> diagnostics;
    std::optional<int> version;
    DomItem doc;
    {
        QMutexLocker l(&m_mutex);
        LastLintUpdate &lastUpdate = m_lastUpdate[url];
        if (lastUpdate.version && *lastUpdate.version == version) {
            qCDebug(lspServerLog) << "skipped update of " << url << "unchanged valid doc";
            return;
        }
        if (snapshot.validDocVersion
            && (!lastUpdate.version || *snapshot.validDocVersion > *lastUpdate.version)) {
            doc = snapshot.validDoc;
            version = snapshot.validDocVersion;
        } else if (snapshot.docVersion
                   && (!lastUpdate.version || *snapshot.docVersion > *lastUpdate.version)) {
            if (!lastUpdate.version || !snapshot.validDocVersion
                || (lastUpdate.invalidUpdatesSince
                    && lastUpdate.invalidUpdatesSince->msecsTo(QDateTime::currentDateTime())
                            > maxInvalidMsec)) {
                doc = snapshot.doc;
                version = snapshot.docVersion;
            } else if (!lastUpdate.invalidUpdatesSince) {
                lastUpdate.invalidUpdatesSince = QDateTime::currentDateTime();
                QTimer::singleShot(maxInvalidMsec, Qt::VeryCoarseTimer, this,
                                   [this, url]() { diagnose(url); });
            }
        }
        if (doc) {
            // update immediately, and do not keep track of sent version, thus in extreme cases sent
            // updates could be out of sync
            lastUpdate.version = version;
            lastUpdate.invalidUpdatesSince.reset();
        }
    }
    QString fileContents;
    if (doc) {
        qCDebug(lintLog) << "has doc, do real lint";
        QStringList imports = m_codeModel->buildPathsForFileUrl(url);
        imports.append(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));
        // add m_server->clientInfo().rootUri & co?
        bool silent = true;
        QString filename = doc.canonicalFilePath();
        fileContents = doc.field(Fields::code).value().toString();
        QStringList qmltypesFiles;
        QStringList resourceFiles;
        QMap<QString, QQmlJSLogger::Option> options;

        QQmlJSLinter linter(imports);

        linter.lintFile(filename, &fileContents, silent, nullptr, imports, qmltypesFiles,
                        resourceFiles, options);
        auto addLength = [&fileContents](Position &position, int startOffset, int length) {
            int i = startOffset;
            int iEnd = i + length;
            if (iEnd > int(fileContents.size()))
                iEnd = fileContents.size();
            while (i < iEnd) {
                if (fileContents.at(i) == u'\n') {
                    ++position.line;
                    position.character = 0;
                    if (i + 1 < iEnd && fileContents.at(i) == u'\r')
                        ++i;
                } else {
                    ++position.character;
                }
                ++i;
            }
        };

        auto messageToDiagnostic = [&addLength, &version](const Message &message) {
            Diagnostic diagnostic;
            diagnostic.severity = severityFromMsgType(message.type);
            Range &range = diagnostic.range;
            Position &position = range.start;

            QQmlJS::SourceLocation srcLoc = message.loc;

            position.line = srcLoc.isValid() ? srcLoc.startLine - 1 : 0;
            position.character = srcLoc.isValid() ? srcLoc.startColumn - 1 : 0;
            range.end = position;
            addLength(range.end, srcLoc.isValid() ? message.loc.offset : 0, srcLoc.isValid() ? message.loc.length : 0);
            diagnostic.message = message.message.toUtf8();
            diagnostic.source = QByteArray("qmllint");

            auto suggestion = message.fixSuggestion;
            if (suggestion.has_value()) {
                // We need to interject the information about where the fix suggestions end
                // here since we don't have access to the textDocument to calculate it later.
                QJsonArray fixedSuggestions;
                for (const FixSuggestion::Fix &fix : suggestion->fixes) {
                    QQmlJS::SourceLocation cut = fix.cutLocation;

                    int line = cut.isValid() ? cut.startLine - 1 : 0;
                    int column = cut.isValid() ? cut.startColumn - 1 : 0;

                    QJsonObject object;
                    object[u"lspBeginLine"] = line;
                    object[u"lspBeginCharacter"] = column;

                    Position end = { line, column };

                    addLength(end, srcLoc.isValid() ? cut.offset : 0,
                              srcLoc.isValid() ? cut.length : 0);
                    object[u"lspEndLine"] = end.line;
                    object[u"lspEndCharacter"] = end.character;

                    object[u"message"] = fix.message;
                    object[u"replacement"] = fix.replacementString;

                    fixedSuggestions << object;
                }
                QJsonObject data;
                data[u"suggestions"] = fixedSuggestions;

                Q_ASSERT(version.has_value());
                data[u"version"] = version.value();

                diagnostic.data = data;
            }
            return diagnostic;
        };
        doc.iterateErrors(
                [&diagnostics, &addLength](DomItem, ErrorMessage msg) {
                    Diagnostic diagnostic;
                    diagnostic.severity = severityFromMsgType(QtMsgType(int(msg.level)));
                    // do something with msg.errorGroups ?
                    auto &location = msg.location;
                    Range &range = diagnostic.range;
                    range.start.line = location.startLine - 1;
                    range.start.character = location.startColumn - 1;
                    range.end = range.start;
                    addLength(range.end, location.offset, location.length);
                    diagnostic.code = QByteArray(msg.errorId.data(), msg.errorId.size());
                    diagnostic.source = "domParsing";
                    diagnostic.message = msg.message.toUtf8();
                    diagnostics.append(diagnostic);
                    return true;
                },
                true);

        if (const QQmlJSLogger *logger = linter.logger()) {
            for (const auto &messages : { logger->infos(), logger->warnings(), logger->errors() }) {
                for (const Message &message : messages) {
                    diagnostics.append(messageToDiagnostic(message));
                }
            }
        }
    }
    PublishDiagnosticsParams diagnosticParams;
    diagnosticParams.uri = url;
    diagnosticParams.diagnostics = diagnostics;
    diagnosticParams.version = version;

    m_server->protocol()->notifyPublishDiagnostics(diagnosticParams);
    qCDebug(lintLog) << "lint" << QString::fromUtf8(url) << "found"
                     << diagnosticParams.diagnostics.size() << "issues"
                     << QTypedJson::toJsonValue(diagnosticParams);
}

} // namespace QmlLsp
QT_END_NAMESPACE
