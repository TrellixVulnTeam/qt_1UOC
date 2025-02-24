// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QSignalSpy>
#include <QDebug>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickpointhandler_p.h>
#include <QtQuick/private/qquickshadereffectsource_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

#include <QtGui/private/qeventpoint_p.h>

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

// On one hand, uncommenting this will make troubleshooting easier (avoid the 60FPS hover events).
// On the other hand, if anything actually breaks when hover events are enabled, that's also a bug.
//#define DISABLE_HOVER_IN_IRRELEVANT_TESTS

struct ViewportTransformHelper : public QQuickDeliveryAgent::Transform
{
    QPointF offset = QPointF(50, 50);

    // Transforms window coordinates to subscene coordinates.
    QPointF map(const QPointF &viewportPoint) override {
        qCDebug(lcTests) << viewportPoint << "->" << viewportPoint - offset;
        return viewportPoint - offset;
    }
};

struct HoverItem : public QQuickItem
{
    HoverItem(QQuickItem *parent) : QQuickItem(parent){}
    void hoverEnterEvent(QHoverEvent *e) override
    {
        hoverEnter = true;
        e->setAccepted(block);
    }

    void hoverLeaveEvent(QHoverEvent *e) override
    {
        hoverLeave = true;
        e->setAccepted(block);
    }

    bool hoverEnter = false;
    bool hoverLeave = false;
    bool block = false;
};

// A QQuick3DViewport simulator
class SubsceneRootItem : public QQuickShaderEffectSource
{
public:
    SubsceneRootItem(QQuickItem *source, QRectF bounds, QQuickItem *parent = nullptr)
        : QQuickShaderEffectSource(parent),
          deliveryAgent(QQuickItemPrivate::get(source)->ensureSubsceneDeliveryAgent())
    {
        setAcceptedMouseButtons(Qt::AllButtons);
        setAcceptTouchEvents(true);
        setAcceptHoverEvents(true);
        setSourceItem(source);
        setSize(bounds.size());
        setPosition(bounds.topLeft());
        setOpacity(0.5);
        deliveryAgent->setObjectName("subscene");
        vxh->offset = position();
    }

    QQuickDeliveryAgent *deliveryAgent = nullptr;

protected:
    bool event(QEvent *e) override {
        if (e->isPointerEvent()) {
            bool ret = false;
            auto pe = static_cast<QPointerEvent *>(e);

            QVarLengthArray<QPointF, 16> originalScenePositions;
            originalScenePositions.resize(pe->pointCount());
            for (int pointIndex = 0; pointIndex < pe->pointCount(); ++pointIndex)
                originalScenePositions[pointIndex] = pe->point(pointIndex).scenePosition();

            for (int pointIndex = 0; pointIndex < pe->pointCount(); ++pointIndex) {
                QEventPoint &p = pe->point(pointIndex);
                QMutableEventPoint::setScenePosition(p, vxh->map(p.scenePosition()));
                QMutableEventPoint::setPosition(p, p.position());
            }

            qCDebug(lcTests) << "forwarding to subscene DA" << pe;
            if (deliveryAgent->event(pe)) {
                ret = true;
                if (QQuickDeliveryAgentPrivate::anyPointGrabbed(pe))
                    deliveryAgent->setSceneTransform(vxh); // takes ownership
            }

            // restore original scene positions
            for (int pointIndex = 0; pointIndex < pe->pointCount(); ++pointIndex)
                QMutableEventPoint::setScenePosition(pe->point(pointIndex), originalScenePositions.at(pointIndex));

            pe->setAccepted(false); // reject implicit grab and let it keep propagating
            qCDebug(lcTests) << e << "returning" << ret;
            return ret;
        } else {
            return QQuickShaderEffectSource::event(e);
        }
    }

    ViewportTransformHelper *vxh = new ViewportTransformHelper;
};

class tst_qquickdeliveryagent : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickdeliveryagent()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
    }

private slots:
    void passiveGrabberOrder();
    void passiveGrabberItems();
    void tapHandlerDoesntOverrideSubsceneGrabber();
    void touchCompression();
    void hoverPropagation_nested_data();
    void hoverPropagation_nested();
    void hoverPropagation_siblings();

private:
    QScopedPointer<QPointingDevice> touchDevice = QScopedPointer<QPointingDevice>(QTest::createTouchDevice());
};

void tst_qquickdeliveryagent::passiveGrabberOrder()
{
    QQuickView view;
    QQmlComponent component(view.engine());
    component.loadUrl(testFileUrl("tapHandler.qml"));
    view.setContent(QUrl(), &component, component.create());
    view.resize(160, 160);
    QQuickItem *root = qobject_cast<QQuickItem*>(view.rootObject());
    QVERIFY(root);
    QQuickTapHandler *rootTap = root->findChild<QQuickTapHandler *>();
    QVERIFY(rootTap);

    QScopedPointer<QQuickItem> subsceneRect(qobject_cast<QQuickItem *>(component.createWithInitialProperties({{"objectName", "child"}})));
    QVERIFY(subsceneRect);
    QQuickTapHandler *subsceneTap = subsceneRect->findChild<QQuickTapHandler *>();
    QVERIFY(subsceneTap);

    SubsceneRootItem subscene(subsceneRect.data(), {50, 50, 100, 100}, view.rootObject());
    QCOMPARE(subsceneRect->parentItem(), nullptr);
    QQuickDeliveryAgent *windowAgent = QQuickWindowPrivate::get(&view)->deliveryAgent;
    windowAgent->setObjectName("window");
    QVERIFY(subscene.deliveryAgent);
    QVERIFY(subscene.deliveryAgent != windowAgent);
    QQuickVisualTestUtils::SignalMultiSpy spy;
    QVERIFY(spy.connectToSignal(rootTap, &QQuickTapHandler::tapped));
    QVERIFY(spy.connectToSignal(subsceneTap, &QQuickTapHandler::tapped));

    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QPoint pos(75, 75);
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, pos);
    QTest::qWait(1000);
    auto devPriv = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice());
    const auto &persistentPoint = devPriv->activePoints.values().first();
    qCDebug(lcTests) << "passive grabbers" << persistentPoint.passiveGrabbers << "contexts" << persistentPoint.passiveGrabbersContext;
    QCOMPARE(persistentPoint.passiveGrabbers.count(), 2);
    QCOMPARE(persistentPoint.passiveGrabbers.first(), subsceneTap);
    QCOMPARE(persistentPoint.passiveGrabbersContext.first(), subscene.deliveryAgent);
    QCOMPARE(persistentPoint.passiveGrabbers.last(), rootTap);

    QTest::mouseRelease(&view, Qt::LeftButton);
    QTest::qWait(100);
    // QQuickWindow::event() has failsafe: clear all grabbers after release
    QCOMPARE(persistentPoint.passiveGrabbers.count(), 0);

    qCDebug(lcTests) << "TapHandlers emitted tapped in this order:" << spy.senders;
    QCOMPARE(spy.senders.count(), 2);
    // passive grabbers are visited in order, and emit tapped() at that time
    QCOMPARE(spy.senders.first(), subsceneTap);
    QCOMPARE(spy.senders.last(), rootTap);
}

class PassiveGrabberItem : public QQuickRectangle
{
public:
    PassiveGrabberItem(QQuickItem *parent = nullptr) : QQuickRectangle(parent) {
        setAcceptedMouseButtons(Qt::LeftButton);
    }
    void mousePressEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << "Passive grabber pressed";
        lastPressed = true;
        event->addPassiveGrabber(event->point(0), this);
        event->ignore();
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << "Mouse move handled by passive grabber";
        const QPointF pos = event->scenePosition();
        const int threshold = 20;
        bool overThreshold = pos.x() >= threshold;
        if (overThreshold) {
            event->setExclusiveGrabber(event->point(0), this);
            this->setKeepMouseGrab(true);
            event->accept();
        } else {
            event->ignore();
        }
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << "Passive grabber released";
        lastPressed = false;
        event->ignore();
    }

    bool lastPressed = false;
};

class ExclusiveGrabberItem : public QQuickRectangle
{
public:
    ExclusiveGrabberItem(QQuickItem *parent = nullptr) : QQuickRectangle(parent) {
        setAcceptedMouseButtons(Qt::LeftButton);
    }
    void mousePressEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << "Exclusive grabber pressed";
        lastPressed = true;
        event->accept();
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        event->accept();
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << "Exclusive grabber released";
        lastPressed = false;
        event->accept();
    }
    void mouseUngrabEvent() override {
        qCDebug(lcTests) << "Exclusive grab ended";
        ungrabbed = true;
    }

    bool lastPressed = false;
    bool ungrabbed = false;
};

void tst_qquickdeliveryagent::passiveGrabberItems()
{
    QQuickView view;
    QQmlComponent component(view.engine());
    qmlRegisterType<PassiveGrabberItem>("Test", 1, 0, "PassiveGrabber");
    qmlRegisterType<ExclusiveGrabberItem>("Test", 1, 0, "ExclusiveGrabber");
    component.loadUrl(testFileUrl("passiveGrabberItem.qml"));
    view.setContent(QUrl(), &component, component.create());
    QQuickItem *root = qobject_cast<QQuickItem*>(view.rootObject());
    QVERIFY(root);
    ExclusiveGrabberItem *exclusiveGrabber = root->property("exclusiveGrabber").value<ExclusiveGrabberItem*>();
    PassiveGrabberItem *passiveGrabber = root->property("passiveGrabber").value<PassiveGrabberItem *>();
    QVERIFY(exclusiveGrabber);
    QVERIFY(passiveGrabber);

    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, QPoint(exclusiveGrabber->x() + 1, exclusiveGrabber->y() + 1));
    auto devPriv = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice());
    const auto &persistentPoint = devPriv->activePoints.values().first();
    QTRY_COMPARE(persistentPoint.passiveGrabbers.count(), 1);
    QCOMPARE(persistentPoint.passiveGrabbers.first(), passiveGrabber);
    QCOMPARE(persistentPoint.exclusiveGrabber, exclusiveGrabber);
    QVERIFY(exclusiveGrabber->lastPressed);
    QVERIFY(passiveGrabber->lastPressed);

    // Mouse move bigger than threshold -> passive grabber becomes exclusive grabber
    QTest::mouseMove(&view);
    QTRY_COMPARE(persistentPoint.exclusiveGrabber, passiveGrabber);
    QVERIFY(exclusiveGrabber->ungrabbed);

    QTest::mouseRelease(&view, Qt::LeftButton);
    // Only the passive grabber got the release event
    // since it became the exclusive grabber on mouseMove
    QTRY_VERIFY(!passiveGrabber->lastPressed);
    QVERIFY(exclusiveGrabber->lastPressed);
    QCOMPARE(persistentPoint.passiveGrabbers.count(), 0);
    QCOMPARE(persistentPoint.exclusiveGrabber, nullptr);

    exclusiveGrabber->lastPressed = false;
    exclusiveGrabber->ungrabbed = false;
    passiveGrabber->lastPressed = false;

    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, QPoint(exclusiveGrabber->x() + 1, exclusiveGrabber->y() + 1));
    const auto &pressedPoint = devPriv->activePoints.values().first();
    QTRY_COMPARE(pressedPoint.passiveGrabbers.count(), 1);
    QCOMPARE(pressedPoint.passiveGrabbers.first(), passiveGrabber);
    QCOMPARE(pressedPoint.exclusiveGrabber, exclusiveGrabber);
    QVERIFY(exclusiveGrabber->lastPressed);
    QVERIFY(passiveGrabber->lastPressed);

    // Mouse move smaller than threshold -> grab remains with the exclusive grabber
    QTest::mouseMove(&view,  QPoint(exclusiveGrabber->x(), exclusiveGrabber->y()));
    QTRY_COMPARE(pressedPoint.exclusiveGrabber, exclusiveGrabber);

    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, QPoint(exclusiveGrabber->x(), exclusiveGrabber->y()));

    // Both the passive and the exclusive grabber get the mouseRelease event
    QTRY_VERIFY(!passiveGrabber->lastPressed);
    QVERIFY(!exclusiveGrabber->lastPressed);
    QCOMPARE(pressedPoint.passiveGrabbers.count(), 0);
    QCOMPARE(pressedPoint.exclusiveGrabber, nullptr);
}

void tst_qquickdeliveryagent::tapHandlerDoesntOverrideSubsceneGrabber() // QTBUG-94012
{
    QQuickView window;
#ifdef DISABLE_HOVER_IN_IRRELEVANT_TESTS
    QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->frameSynchronousHoverEnabled = false;
#endif
    QVERIFY(QQuickTest::initView(window, testFileUrl("flickableTextEdit.qml")));
    QQuickItem *textEdit = window.rootObject()->findChild<QQuickItem*>("textEdit");
    QVERIFY(textEdit);
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);

    // put the Flickable into a SubsceneRootItem
    SubsceneRootItem subscene(flickable, flickable->boundingRect().translated(flickable->width() + 20, 10), window.rootObject());
    QPoint clickPos = subscene.boundingRect().translated(subscene.width(), 10).center().toPoint();

    // add a TapHandler to it
    QQuickTapHandler tapHandler(&subscene);
    QSignalSpy clickSpy(&tapHandler, &QQuickTapHandler::tapped);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    int cursorPos = textEdit->property("cursorPosition").toInt();

    // Click on the middle of the subscene to the right (texture cloned from the left).
    // TapHandler takes a passive grab on press; TextEdit takes the exclusive grab;
    // and TapHandler does not emit tapped, because of the non-filtering exclusive grabber.
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, clickPos);
    qCDebug(lcTests) << "clicking subscene TextEdit set cursorPos to" << cursorPos;
    QVERIFY(textEdit->property("cursorPosition").toInt() > cursorPos);
    QCOMPARE(clickSpy.count(), 0); // doesn't tap
}

void tst_qquickdeliveryagent::touchCompression()
{
    QQuickView window;
    // avoid interference from X11 window managers, so we can look at eventpoint globalPosition
    window.setFlag(Qt::FramelessWindowHint);
#ifdef DISABLE_HOVER_IN_IRRELEVANT_TESTS
    QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->frameSynchronousHoverEnabled = false;
#endif
    QVERIFY(QQuickTest::showView(window, testFileUrl("pointHandler.qml")));
    QQuickDeliveryAgent *windowAgent = QQuickWindowPrivate::get(&window)->deliveryAgent;
    QQuickDeliveryAgentPrivate *agentPriv = static_cast<QQuickDeliveryAgentPrivate *>(QQuickDeliveryAgentPrivate::get(windowAgent));
    QQuickItem *root = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(root);
    QQuickPointHandler *rootHandler = root->findChild<QQuickPointHandler *>();
    QVERIFY(rootHandler);
    QTest::QTouchEventSequence touch = QTest::touchEvent(&window, touchDevice.data());
    QPoint pt1(30, 50);
    QPoint pt2(70, 50);
    // Press and drag fast, alternating moving and stationary points
    touch.press(11, pt1).press(12, pt2).commit();
    QQuickTouchUtils::flush(&window);
    QTest::qWait(50); // not critical, but let it hopefully render a frame or two
    QCOMPARE(agentPriv->compressedTouchCount, 0);
    for (int m = 1; m < 4; ++m) {
        pt1 += {0, 1};
        pt2 -= {0, 1};
        if (m % 2)
            touch.move(11, pt1).stationary(12).commit();
        else
            touch.stationary(11).move(12, pt2).commit();
        // don't call QQuickTouchUtils::flush() here: we want to see the compression happen
        if (agentPriv->compressedTouchCount) {
            if (m % 2) {
                QCOMPARE(agentPriv->delayedTouch->point(0).position().toPoint(), pt1);
                QCOMPARE(agentPriv->delayedTouch->point(0).globalPosition().toPoint(), root->mapToGlobal(pt1).toPoint());
            } else {
                QCOMPARE(agentPriv->delayedTouch->point(1).position().toPoint(), pt2);
                QCOMPARE(agentPriv->delayedTouch->point(1).globalPosition().toPoint(), root->mapToGlobal(pt2).toPoint());
            }
        }
        // we can't guarantee that a CI VM is fast enough, but usually compressedTouchCount == m
        qCDebug(lcTests) << "compressedTouchCount" << agentPriv->compressedTouchCount << "expected" << m;
        qCDebug(lcTests) << "PointHandler still sees" << rootHandler->point().position() << "while" << pt1 << "was likely not yet delivered";
    }
    QTRY_COMPARE(rootHandler->point().position().toPoint(), pt1);
    touch.release(11, pt1).release(12, pt2).commit();
    // should be delivered, bypassing compression; when PointHandler gets the release, it will reset its point
    QTRY_COMPARE(rootHandler->active(), false);
    QCOMPARE(rootHandler->point().position(), QPointF());
    QCOMPARE(agentPriv->compressedTouchCount, 0);
}

void tst_qquickdeliveryagent::hoverPropagation_nested_data()
{
    QTest::addColumn<bool>("block");
    QTest::newRow("block=false") << false;
    QTest::newRow("block=true") << true;
}

void tst_qquickdeliveryagent::hoverPropagation_nested()
{
    QFETCH(bool, block);

    QQuickWindow window;
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    HoverItem child(window.contentItem());
    child.setAcceptHoverEvents(true);
    child.setWidth(100);
    child.setHeight(100);

    HoverItem grandChild(&child);
    grandChild.setAcceptHoverEvents(true);
    grandChild.block = block;
    grandChild.setWidth(100);
    grandChild.setHeight(100);

    // Start by moving the mouse to the window
    QTest::mouseMove(&window, QPoint(150, 150));
    QCOMPARE(child.hoverEnter, false);
    QCOMPARE(grandChild.hoverEnter, false);

    // Move the mouse inside the items. If block is true, only
    // the grandchild should be hovered. Otherwise both.
    QTest::mouseMove(&window, QPoint(50, 50));
    QCOMPARE(child.hoverEnter, !block);
    QCOMPARE(grandChild.hoverEnter, true);
}

void tst_qquickdeliveryagent::hoverPropagation_siblings()
{
    QQuickWindow window;
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    HoverItem sibling1(window.contentItem());
    sibling1.setAcceptHoverEvents(true);
    sibling1.setWidth(100);
    sibling1.setHeight(100);

    HoverItem sibling2(window.contentItem());
    sibling2.setAcceptHoverEvents(true);
    sibling2.setWidth(100);
    sibling2.setHeight(100);

    // Start by moving the mouse to the window
    QTest::mouseMove(&window, QPoint(150, 150));
    QCOMPARE(sibling1.hoverEnter, false);
    QCOMPARE(sibling2.hoverEnter, false);

    // Move the mouse inside the items. Only the
    // sibling on the top should receive hover
    QTest::mouseMove(&window, QPoint(50, 50));
    QCOMPARE(sibling1.hoverEnter, false);
    QCOMPARE(sibling2.hoverEnter, true);
}

QTEST_MAIN(tst_qquickdeliveryagent)

#include "tst_qquickdeliveryagent.moc"
