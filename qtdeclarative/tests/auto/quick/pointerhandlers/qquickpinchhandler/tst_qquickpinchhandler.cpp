// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtGui/QStyleHints>
#include <QtGui/private/qeventpoint_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtQuick/private/qquickpinchhandler_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class PinchHandler : public QQuickPinchHandler {
public:
    const QQuickHandlerPoint &firstPoint() { return currentPoints().first(); }
    const QQuickHandlerPoint &lastPoint() { return currentPoints().last(); }
};

class tst_QQuickPinchHandler: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickPinchHandler() : QQmlDataTest(QT_QMLTEST_DATADIR) { }
private slots:
    void cleanupTestCase();
    void pinchProperties();
    void scale();
    void scaleThreeFingers();
    void scaleNativeGesture_data();
    void scaleNativeGesture();
    void pan();
    void dragAxesEnabled_data();
    void dragAxesEnabled();
    void retouch();
    void cancel();
    void transformedpinchHandler_data();
    void transformedpinchHandler();

private:
    QPointingDevice *touchscreen = QTest::createTouchDevice();
    QPointingDevice *touchpad = QTest::createTouchDevice(QInputDevice::DeviceType::TouchPad);
};

void tst_QQuickPinchHandler::cleanupTestCase()
{

}

static bool withinBounds(qreal lower, qreal num, qreal upper)
{
    return num >= lower && num <= upper;
}

void tst_QQuickPinchHandler::pinchProperties()
{
    QScopedPointer<QQuickView> window(QQuickViewTestUtils::createView());
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(window->rootObject() != nullptr);

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, pinchHandler->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(rootItem != nullptr);
    QSignalSpy targetSpy(pinchHandler, SIGNAL(targetChanged()));
    pinchHandler->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);
    pinchHandler->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);

    // axis
    /*
    QCOMPARE(pinchHandler->axis(), QQuickPinch::XAndYAxis);
    QSignalSpy axisSpy(pinchHandler, SIGNAL(dragAxisChanged()));
    pinchHandler->setAxis(QQuickPinch::XAxis);
    QCOMPARE(pinchHandler->axis(), QQuickPinch::XAxis);
    QCOMPARE(axisSpy.count(),1);
    pinchHandler->setAxis(QQuickPinch::XAxis);
    QCOMPARE(axisSpy.count(),1);

    // minimum and maximum drag properties
    QSignalSpy xminSpy(pinchHandler, SIGNAL(minimumXChanged()));
    QSignalSpy xmaxSpy(pinchHandler, SIGNAL(maximumXChanged()));
    QSignalSpy yminSpy(pinchHandler, SIGNAL(minimumYChanged()));
    QSignalSpy ymaxSpy(pinchHandler, SIGNAL(maximumYChanged()));

    QCOMPARE(pinchHandler->xmin(), 0.0);
    QCOMPARE(pinchHandler->xmax(), rootItem->width()-blackRect->width());
    QCOMPARE(pinchHandler->ymin(), 0.0);
    QCOMPARE(pinchHandler->ymax(), rootItem->height()-blackRect->height());

    pinchHandler->setXmin(10);
    pinchHandler->setXmax(10);
    pinchHandler->setYmin(10);
    pinchHandler->setYmax(10);

    QCOMPARE(pinchHandler->xmin(), 10.0);
    QCOMPARE(pinchHandler->xmax(), 10.0);
    QCOMPARE(pinchHandler->ymin(), 10.0);
    QCOMPARE(pinchHandler->ymax(), 10.0);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    pinchHandler->setXmin(10);
    pinchHandler->setXmax(10);
    pinchHandler->setYmin(10);
    pinchHandler->setYmax(10);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);
    */

    // minimum and maximum scale properties
    QSignalSpy scaleMinSpy(pinchHandler, SIGNAL(minimumScaleChanged()));
    QSignalSpy scaleMaxSpy(pinchHandler, SIGNAL(maximumScaleChanged()));

    QCOMPARE(pinchHandler->minimumScale(), 0.5);
    QCOMPARE(pinchHandler->maximumScale(), 4.0);

    pinchHandler->setMinimumScale(0.25);
    pinchHandler->setMaximumScale(1.5);

    QCOMPARE(pinchHandler->minimumScale(), 0.25);
    QCOMPARE(pinchHandler->maximumScale(), 1.5);

    QCOMPARE(scaleMinSpy.count(),1);
    QCOMPARE(scaleMaxSpy.count(),1);

    pinchHandler->setMinimumScale(0.25);
    pinchHandler->setMaximumScale(1.5);

    QCOMPARE(scaleMinSpy.count(),1);
    QCOMPARE(scaleMaxSpy.count(),1);

    // minimum and maximum rotation properties
    QSignalSpy rotMinSpy(pinchHandler, SIGNAL(minimumRotationChanged()));
    QSignalSpy rotMaxSpy(pinchHandler, SIGNAL(maximumRotationChanged()));

    QCOMPARE(pinchHandler->minimumRotation(), 0.0);
    QCOMPARE(pinchHandler->maximumRotation(), 90.0);

    pinchHandler->setMinimumRotation(-90.0);
    pinchHandler->setMaximumRotation(45.0);

    QCOMPARE(pinchHandler->minimumRotation(), -90.0);
    QCOMPARE(pinchHandler->maximumRotation(), 45.0);

    QCOMPARE(rotMinSpy.count(),1);
    QCOMPARE(rotMaxSpy.count(),1);

    pinchHandler->setMinimumRotation(-90.0);
    pinchHandler->setMaximumRotation(45.0);

    QCOMPARE(rotMinSpy.count(),1);
    QCOMPARE(rotMaxSpy.count(),1);
}

QEventPoint makeTouchPoint(int id, QPoint p, QQuickView *v, QQuickItem *i)
{
    QEventPoint touchPoint(id);
    QMutableEventPoint::setPosition(touchPoint, i->mapFromScene(p));
    QMutableEventPoint::setGlobalPosition(touchPoint, v->mapToGlobal(p));
    QMutableEventPoint::setScenePosition(touchPoint, p);
    return touchPoint;
}

void tst_QQuickPinchHandler::scale()
{
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    auto *pinchHandler = static_cast<PinchHandler *>(window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler"));
    QVERIFY(pinchHandler != nullptr);
    QSignalSpy grabChangedSpy(pinchHandler, SIGNAL(grabChanged(QPointingDevice::GrabTransition, QEventPoint)));

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p0(80, 80);
    QPoint p1(100, 100);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen);
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.  Otherwise if we let it
        // be destroyed and then start a new sequence, point 0 will default to being
        // stationary at 0, 0, and pinchHandler will filter out that touchpoint because
        // it is outside its bounds.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QTRY_COMPARE(grabChangedSpy.count(), 1); // passive grab

        QPoint pd(10, 10);
        // move one point until PinchHandler activates
        for (int pi = 0; pi < 10 && !pinchHandler->active(); ++pi) {
            p1 += pd;
            pinchSequence.stationary(0).move(1, p1, window).commit();
            QQuickTouchUtils::flush(window);
        }
        QCOMPARE(pinchHandler->active(), true);
        // grabs occur when the handler becomes active; at that time, QQuickHandlerPoint.sceneGrabPosition should be correct
        QVERIFY(pinchHandler->firstPoint().sceneGrabPosition() != QPointF());
        QVERIFY(pinchHandler->lastPoint().sceneGrabPosition() != QPointF());
        QCOMPARE(pinchHandler->firstPoint().sceneGrabPosition(), pinchHandler->firstPoint().scenePosition());
        QCOMPARE(pinchHandler->lastPoint().sceneGrabPosition(), pinchHandler->lastPoint().scenePosition());
        // first point got a passive grab; both points got exclusive grabs
        QCOMPARE(grabChangedSpy.count(), 3);
        QLineF line(p0, p1);
        const qreal startLength = line.length();

        p1+=pd;
        pinchSequence.stationary(0).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        line.setP2(p1);
        qreal scale = line.length() / startLength;
        QVERIFY(qFloatDistance(root->property("scale").toReal(), scale) < 10);
        QVERIFY(qFloatDistance(blackRect->scale(), scale) < 10);

        p1+=pd;
        pinchSequence.stationary(0).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        line.setP2(p1);
        scale = line.length() / startLength;

        QVERIFY(qFloatDistance(root->property("scale").toReal(), scale) < 10);
        QVERIFY(qFloatDistance(blackRect->scale(), scale) < 10);

        QPointF expectedCentroid = p0 + (p1 - p0)/2;
        QCOMPARE(pinchHandler->centroid().scenePosition(), expectedCentroid);
    }

    // scale beyond bound
    p1 += QPoint(20, 20);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen);
        pinchSequence.stationary(0).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(blackRect->scale(), qreal(4));    // qquickpinchhandler does not manipulate scale property
        pinchSequence.release(0, p0, window).release(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
    }
    QCOMPARE(pinchHandler->active(), false);
}

void tst_QQuickPinchHandler::scaleThreeFingers()
{
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("threeFingers.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    // center of blackrect is at 150,150
    QPoint p0(80, 80);
    QPoint p1(220, 80);
    QPoint p2(150, 220);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen);
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.  Otherwise if we let it
        // be destroyed and then start a new sequence, point 0 will default to being
        // stationary at 0, 0, and pinchHandler will filter out that touchpoint because
        // it is outside its bounds.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        pinchSequence.stationary(0).stationary(1).press(2, p2, window).commit();
        QQuickTouchUtils::flush(window);
        for (int i = 0; i < 5;++i) {
            p0 += QPoint(-4, -4);
            p1 += QPoint(+4, -4);
            p2 += QPoint( 0, +6);
            pinchSequence.move(0, p0,window).move(1, p1,window).move(2, p2,window).commit();
            QQuickTouchUtils::flush(window);
        }

        QCOMPARE(pinchHandler->active(), true);
        // scale we got was 1.1729088738267854364, but keep some slack
        QVERIFY(withinBounds(1.163, root->property("scale").toReal(), 1.183));
        // should not rotate
        QCOMPARE(root->property("rotation").toReal(), 0.);

        for (int i = 0; i < 5;++i) {
            p0 += QPoint(-4, -4);
            p1 += QPoint(+4, -4);
            p2 += QPoint( 0, +6);
            pinchSequence.move(0, p0,window).move(1, p1,window).move(2, p2,window).commit();
            QQuickTouchUtils::flush(window);
        }
        // scale we got was 1.4613, but keep some slack
        QVERIFY(withinBounds(1.361, root->property("scale").toReal(), 1.561));

        // since points were moved symetrically around the y axis, centroid should remain at x:150
        QCOMPARE(root->property("centroid").value<QQuickHandlerPoint>().scenePosition().x(), 150); // blackrect is at 50,50

        // scale beyond bound, we should reach the maximumScale
        p0 += QPoint(-40, -40);
        p1 += QPoint(+40, -40);
        p2 += QPoint(  0, +60);
        pinchSequence.move(0, p0,window).move(1, p1,window).move(2, p2,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 2.);
        pinchSequence.release(0, p0, window).release(1, p1, window).release(2, p2, window).commit();
        QQuickTouchUtils::flush(window);
    }
    QCOMPARE(pinchHandler->active(), false);
}

void tst_QQuickPinchHandler::scaleNativeGesture_data()
{
    QTest::addColumn<QString>("qmlfile");
    QTest::addColumn<qreal>("scale");

    QTest::newRow("just pinch") << "pinchproperties.qml" << 1.1;
    QTest::newRow("pinch & drag") << "pinchAndDrag.qml" << 1.1;
    QTest::newRow("bigger than limit") << "pinchproperties.qml" << 5.0;
    QTest::newRow("smaller than limit") << "pinchproperties.qml" << 0.25;
}

void tst_QQuickPinchHandler::scaleNativeGesture()
{
    QFETCH(QString, qmlfile);
    QFETCH(qreal, scale);

    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl(qmlfile));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);
    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);
    QQuickItem *target = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(target != nullptr);

    QPointF targetPos = target->position();
    ulong ts = 1;

    // first pinch: scale it
    const qreal expectedScale = qBound(qreal(0.5), scale, qreal(4));
    QPointF pinchPos(75, 75);
    QPointF pinchLocalPos = target->mapFromScene(pinchPos);
    // target position is adjusted in QQuickItemPrivate::adjustedPosForTransform()
    // so as to compensate for the change in size, to hold the centroid in place
    const QPointF expectedPos = targetPos + QPointF( (pinchPos.x() - target->x()) * (expectedScale - 1),
                                                     (pinchPos.y() - target->y()) * (expectedScale - 1) );
    QWindowSystemInterface::handleGestureEvent(window, ts++, touchpad,
                                               Qt::BeginNativeGesture, pinchPos, pinchPos);
    QWindowSystemInterface::handleGestureEventWithRealValue(window, ts++, touchpad,
                                                            Qt::ZoomNativeGesture, scale - 1, pinchPos, pinchPos);
    QTRY_COMPARE(target->scale(), expectedScale);
    QCOMPARE(pinchHandler->active(), true);
    QCOMPARE(pinchHandler->centroid().position().toPoint(), pinchLocalPos.toPoint());
    QCOMPARE(pinchHandler->centroid().scenePosition().toPoint(), pinchPos.toPoint());
    QVERIFY(qAbs(target->position().x() - expectedPos.x()) < 0.001);
    QVERIFY(qAbs(target->position().y() - expectedPos.y()) < 0.001);
    QCOMPARE(pinchHandler->scale(), expectedScale);
    QCOMPARE(pinchHandler->activeScale(), expectedScale);
    QCOMPARE(pinchHandler->translation(), QVector2D());
    QCOMPARE(pinchHandler->rotation(), 0);
    QWindowSystemInterface::handleGestureEvent(window, ts++, touchpad,
                                               Qt::EndNativeGesture, pinchPos, pinchPos);
    QTRY_COMPARE(pinchHandler->active(), false);
    QCOMPARE(target->scale(), expectedScale);
    QCOMPARE(pinchHandler->scale(), expectedScale);
    QCOMPARE(pinchHandler->activeScale(), 1);
    QCOMPARE(pinchHandler->translation(), QVector2D());
    QCOMPARE(pinchHandler->rotation(), 0);

    // second pinch at a different position: scale it back to original size again
    // but remove the limits first, so that we can scale arbitrarily
    pinchHandler->setMaximumScale(qInf());
    pinchHandler->setMinimumScale(-qInf());
    const qreal reverseScale = (1 / expectedScale);
    pinchPos = QPointF(110, 110);
    pinchLocalPos = target->mapFromScene(pinchPos);
    QWindowSystemInterface::handleGestureEvent(window, ts++, touchpad,
                                               Qt::BeginNativeGesture, pinchPos, pinchPos);
    QWindowSystemInterface::handleGestureEventWithRealValue(window, ts++, touchpad,
                                                            Qt::ZoomNativeGesture, reverseScale - 1, pinchPos, pinchPos);
    QTRY_COMPARE(target->scale(), 1);
    QCOMPARE(pinchHandler->active(), true);
    QCOMPARE(pinchHandler->centroid().position().toPoint(), pinchLocalPos.toPoint());
    QCOMPARE(pinchHandler->centroid().scenePosition().toPoint(), pinchPos.toPoint());
    QCOMPARE(pinchHandler->scale(), 1);
    QCOMPARE(pinchHandler->activeScale(), reverseScale);
    QWindowSystemInterface::handleGestureEvent(window, ts++, touchpad,
                                               Qt::EndNativeGesture, pinchPos, pinchPos);
    QTRY_COMPARE(pinchHandler->active(), false);
    QCOMPARE(target->scale(), 1);
    QCOMPARE(pinchHandler->scale(), 1);
    QCOMPARE(pinchHandler->activeScale(), 1);
}

void tst_QQuickPinchHandler::pan()
{
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p0(80, 80);
    QPoint p1(100, 100);
    {
        const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen);
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QVERIFY(!root->property("pinchActive").toBool());
        QCOMPARE(root->property("scale").toReal(), -1.0);

        p0 += QPoint(dragThreshold, 0);
        p1 += QPoint(dragThreshold, 0);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        // movement < dragThreshold: pinchHandler not yet active
        QVERIFY(!root->property("pinchActive").toBool());
        QCOMPARE(root->property("scale").toReal(), -1.0);

        // just above the dragThreshold: pinchHandler starts
        p0 += QPoint(1, 0);
        p1 += QPoint(1, 0);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(pinchHandler->active(), true);
        QCOMPARE(root->property("scale").toReal(), 1.0);

        // Calculation of the center point is tricky at first:
        // center point of the two touch points in item coordinates:
        // scene coordinates: (80, 80) + (dragThreshold, 0), (100, 100) + (dragThreshold, 0)
        //                    = ((180+dT)/2, 180/2) = (90+dT, 90)
        // item  coordinates: (scene) - (50, 50) = (40+dT, 40)
        QCOMPARE(pinchHandler->centroid().scenePosition(), QPointF(90 + dragThreshold + 1, 90));
        // pan started, but no actual movement registered yet:
        // blackrect starts at 50,50
        QCOMPARE(blackRect->x(), 50.0);
        QCOMPARE(blackRect->y(), 50.0);

        p0 += QPoint(10, 0);
        p1 += QPoint(10, 0);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(pinchHandler->centroid().scenePosition(), QPointF(90 + dragThreshold + 11, 90));
        QCOMPARE(blackRect->x(), 60.0);
        QCOMPARE(blackRect->y(), 50.0);

        p0 += QPoint(0, 10);
        p1 += QPoint(0, 10);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(pinchHandler->centroid().scenePosition(), QPointF(90 + dragThreshold + 11, 90 + 10));
        QCOMPARE(blackRect->x(), 60.0);
        QCOMPARE(blackRect->y(), 60.0);

        p0 += QPoint(10, 10);
        p1 += QPoint(10, 10);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        // now the item moved again, thus the center point of the touch is moved in total by (10, 10)
        QCOMPARE(pinchHandler->centroid().scenePosition(), QPointF(90 + dragThreshold + 21, 90 + 20));
        QCOMPARE(blackRect->x(), 70.0);
        QCOMPARE(blackRect->y(), 70.0);
    }

    // pan x beyond bound
    p0 += QPoint(100,100);
    p1 += QPoint(100,100);
    QTest::touchEvent(window, touchscreen).move(0, p0, window).move(1, p1, window);
    QQuickTouchUtils::flush(window);

    QCOMPARE(blackRect->x(), 140.0);
    QCOMPARE(blackRect->y(), 170.0);

    QTest::touchEvent(window, touchscreen).release(0, p0, window).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!root->property("pinchActive").toBool());
}

void tst_QQuickPinchHandler::dragAxesEnabled_data()
{
    QTest::addColumn<bool>("xEnabled");
    QTest::addColumn<bool>("yEnabled");

    QTest::newRow("both enabled") << true << true;
    QTest::newRow("x enabled") << true << false;
    QTest::newRow("y enabled") << false << true;
    QTest::newRow("both disabled") << false << false;
}

void tst_QQuickPinchHandler::dragAxesEnabled()
{
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QQuickPinchHandler *pinchHandler = blackRect->findChild<QQuickPinchHandler*>();
    QVERIFY(pinchHandler != nullptr);

    QFETCH(bool, xEnabled);
    QFETCH(bool, yEnabled);
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    pinchHandler->xAxis()->setEnabled(xEnabled);
    pinchHandler->yAxis()->setEnabled(yEnabled);
    QPoint c = blackRect->mapToScene(blackRect->clipRect().center()).toPoint();
    QPoint p0 = c - QPoint(0, dragThreshold);
    QPoint p1 = c + QPoint(0, dragThreshold);
    QPoint blackRectPos = blackRect->position().toPoint();

    // press two points, one above the rectangle's center and one below
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen);
    pinchSequence.press(0, p0, window).press(1, p1, window).commit();
    QQuickTouchUtils::flush(window);

    // expand the pinch vertically
    p0 -= QPoint(0, dragThreshold);
    p1 += QPoint(0, dragThreshold);
    pinchSequence.move(0, p0, window).move(1, p1, window).commit();
    for (int pi = 0; pi < 4; ++pi) {
        p0 -= QPoint(0, dragThreshold);
        p1 += QPoint(0, dragThreshold);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        qCDebug(lcPointerTests) << pi << "active" << pinchHandler->active() << "pts" << p0 << p1
                                << "centroid" << pinchHandler->centroid().scenePosition()
                                << "rect pos" << blackRect->position() << "scale" << blackRect->scale();
    }
    QCOMPARE(pinchHandler->active(), true);
    QVERIFY(blackRect->scale() >= 2.0);
    // drag started, but we only did scaling without any translation
    QCOMPARE(pinchHandler->centroid().scenePosition().toPoint(), c);
    QCOMPARE(blackRect->position().toPoint().x(), blackRectPos.x());
    QCOMPARE(blackRect->position().toPoint().y(), blackRectPos.y());

    // drag diagonally
    p0 += QPoint(150, 150);
    p1 += QPoint(150, 150);
    pinchSequence.move(0, p0, window).move(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    // the target should move if the xAxis is enabled, or stay in place if not
    qCDebug(lcPointerTests) << "after diagonal drag: pts" << p0 << p1
                            << "centroid" << pinchHandler->centroid().scenePosition()
                            << "rect pos" << blackRect->position() << "scale" << blackRect->scale();
    QCOMPARE(pinchHandler->centroid().scenePosition().toPoint(), QPoint(250, 250));
    QCOMPARE(blackRect->position().toPoint().x(), xEnabled ? 140 : blackRectPos.x()); // because of xAxis.maximum
    QCOMPARE(blackRect->position().toPoint().y(), yEnabled ? 170 : blackRectPos.y()); // because of yAxis.maximum

    QTest::touchEvent(window, touchscreen).release(0, p0, window).release(1, p1, window);
    QQuickTouchUtils::flush(window);
}

// test pinchHandler, release one point, touch again to continue pinchHandler
void tst_QQuickPinchHandler::retouch()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p0(80, 80);
    QPoint p1(100, 100);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen);
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        const QPoint delta(dragThreshold + 1, dragThreshold + 1);
        p0 -= delta;
        p1 += delta;
        pinchSequence.move(0, p0,window).move(1, p1,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.0);
        QCOMPARE(pinchHandler->active(), true);

        p0 -= delta;
        p1 += delta;
        pinchSequence.move(0, p0,window).move(1, p1,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(pinchHandler->active(), true);

        // accept some slack
        QVERIFY(withinBounds(1.4, root->property("scale").toReal(), 1.6));
        QCOMPARE(pinchHandler->centroid().position().toPoint(), QPoint(40, 40)); // blackrect is at 50,50
        QVERIFY(withinBounds(1.4, blackRect->scale(), 1.6));

        QCOMPARE(root->property("activeCount").toInt(), 1);
        QCOMPARE(root->property("deactiveCount").toInt(), 0);

        // Hold down the first finger but release the second one
        pinchSequence.stationary(0).release(1, p1, window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("activeCount").toInt(), 1);
        QCOMPARE(root->property("deactiveCount").toInt(), 1);

        // Keep holding down the first finger and re-touch the second one, then move them both
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        p0 -= QPoint(10,10);
        p1 += QPoint(10,10);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);

        // Lifting and retouching results in onPinchStarted being called again
        QCOMPARE(root->property("activeCount").toInt(), 2);
        QCOMPARE(root->property("deactiveCount").toInt(), 1);

        pinchSequence.release(0, p0, window).release(1, p1, window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(pinchHandler->active(), false);
        QCOMPARE(root->property("activeCount").toInt(), 2);
        QCOMPARE(root->property("deactiveCount").toInt(), 2);
    }
}

void tst_QQuickPinchHandler::cancel()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p0(80, 80);
    QPoint p1(100, 100);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen);
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.  Otherwise if we let it
        // be destroyed and then start a new sequence, point 0 will default to being
        // stationary at 0, 0, and pinchHandler will filter out that touchpoint because
        // it is outside its bounds.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        const QPoint delta(dragThreshold + 1, dragThreshold + 1);
        p0 -= delta;
        p1 += delta;
        pinchSequence.move(0, p0,window).move(1, p1,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.0);
        QCOMPARE(pinchHandler->active(), true);

        p0 -= delta;
        p1 += delta;
        pinchSequence.move(0, p0,window).move(1, p1,window).commit();
        QQuickTouchUtils::flush(window);

        QVERIFY(withinBounds(1.4, root->property("scale").toReal(), 1.6));
        QCOMPARE(pinchHandler->centroid().position().toPoint(), QPoint(40, 40)); // blackrect is at 50,50
        QVERIFY(withinBounds(1.4, blackRect->scale(), 1.6));

        QSKIP("cancel is not supported atm");

        QTouchEvent cancelEvent(QEvent::TouchCancel, touchscreen);
        QCoreApplication::sendEvent(window, &cancelEvent);
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.0);
        QCOMPARE(root->property("center").toPoint(), QPoint(40, 40)); // blackrect is at 50,50
        QCOMPARE(blackRect->scale(), 1.0);
        QVERIFY(!root->property("pinchActive").toBool());
    }
}

void tst_QQuickPinchHandler::transformedpinchHandler_data()
{
    QTest::addColumn<QPoint>("p0");
    QTest::addColumn<QPoint>("p1");
    QTest::addColumn<bool>("shouldPinch");

    QTest::newRow("checking inner pinchHandler 1")
        << QPoint(200, 140) << QPoint(200, 260) << true;

    QTest::newRow("checking inner pinchHandler 2")
        << QPoint(140, 200) << QPoint(200, 140) << true;

    QTest::newRow("checking inner pinchHandler 3")
        << QPoint(140, 200) << QPoint(260, 200) << true;

    QTest::newRow("checking outer pinchHandler 1")
        << QPoint(140, 140) << QPoint(260, 260) << false;

    QTest::newRow("checking outer pinchHandler 2")
        << QPoint(140, 140) << QPoint(200, 200) << false;

    QTest::newRow("checking outer pinchHandler 3")
        << QPoint(140, 260) << QPoint(260, 260) << false;
}

void tst_QQuickPinchHandler::transformedpinchHandler()
{
    QFETCH(QPoint, p0);
    QFETCH(QPoint, p1);
    QFETCH(bool, shouldPinch);

    QQuickView *view = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(view);
    view->setSource(testFileUrl("transformedPinchHandler.qml"));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    QVERIFY(view->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = view->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    const int threshold = qApp->styleHints()->startDragDistance();

    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(view, touchscreen);
        // start pinchHandler
        pinchSequence.press(0, p0, view).commit();
        QQuickTouchUtils::flush(view);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p1, view).commit();
        QQuickTouchUtils::flush(view);

        // we move along the line that the two points form.
        // The distance we move should be above the threshold (threshold * 2 to be safe)
        QVector2D delta(p1 - p0);
        delta.normalize();
        QVector2D movement = delta * (threshold * 2);
        pinchSequence.stationary(0).move(1, p1 + movement.toPoint(), view).commit();
        QQuickTouchUtils::flush(view);
        QCOMPARE(pinchHandler->active(), shouldPinch);

        // release pinchHandler
        pinchSequence.release(0, p0, view).release(1, p1, view).commit();
        QQuickTouchUtils::flush(view);
        QCOMPARE(pinchHandler->active(), false);
    }
}

QTEST_MAIN(tst_QQuickPinchHandler)

#include "tst_qquickpinchhandler.moc"
