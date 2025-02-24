// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qtwistsensorgesturerecognizer.h"

#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

#define TIMER_TIMEOUT 750
QTwistSensorGestureRecognizer::QTwistSensorGestureRecognizer(QObject *parent)
    : QSensorGestureRecognizer(parent)
    , orientationReading(0)
    , active(0)
    , detecting(0)
    , checking(0)
    , increaseCount(0)
    , decreaseCount(0)
    , lastAngle(0)
    , detectedAngle(0)
{
}

QTwistSensorGestureRecognizer::~QTwistSensorGestureRecognizer()
{
}

void QTwistSensorGestureRecognizer::create()
{
}

QString QTwistSensorGestureRecognizer::id() const
{
    return QString("QtSensors.twist");
}

bool QTwistSensorGestureRecognizer::start()
{
    if (QtSensorGestureSensorHandler::instance()->startSensor(QtSensorGestureSensorHandler::Accel)) {
        if (QtSensorGestureSensorHandler::instance()->startSensor(QtSensorGestureSensorHandler::Orientation)) {
            active = true;
            connect(QtSensorGestureSensorHandler::instance(),SIGNAL(orientationReadingChanged(QOrientationReading*)),
                    this,SLOT(orientationReadingChanged(QOrientationReading*)));

            connect(QtSensorGestureSensorHandler::instance(),SIGNAL(accelReadingChanged(QAccelerometerReading*)),
                    this,SLOT(accelChanged(QAccelerometerReading*)));
        } else {
            QtSensorGestureSensorHandler::instance()->stopSensor(QtSensorGestureSensorHandler::Accel);
            active = false;
        }
    } else {

        active = false;
    }

    return active;
}

bool QTwistSensorGestureRecognizer::stop()
{
    QtSensorGestureSensorHandler::instance()->stopSensor(QtSensorGestureSensorHandler::Accel);
    QtSensorGestureSensorHandler::instance()->stopSensor(QtSensorGestureSensorHandler::Orientation);
    disconnect(QtSensorGestureSensorHandler::instance(),SIGNAL(orientationReadingChanged(QOrientationReading*)),
            this,SLOT(orientationReadingChanged(QOrientationReading*)));

    disconnect(QtSensorGestureSensorHandler::instance(),SIGNAL(accelReadingChanged(QAccelerometerReading*)),
            this,SLOT(accelChanged(QAccelerometerReading*)));

    reset();
    orientationList.clear();
    active = false;
    return active;
}

bool QTwistSensorGestureRecognizer::isActive()
{
    return active;
}

void QTwistSensorGestureRecognizer::orientationReadingChanged(QOrientationReading *reading)
{
    orientationReading = reading;
    if (orientationList.count() == 3)
        orientationList.removeFirst();

    orientationList.append(reading->orientation());

    if (orientationList.count() == 3
            &&  orientationList.at(2) == QOrientationReading::FaceUp
            && (orientationList.at(1) == QOrientationReading::RightUp
                || orientationList.at(1) == QOrientationReading::LeftUp)) {
        checkTwist();
    }

    checkOrientation();
}

bool QTwistSensorGestureRecognizer::checkOrientation()
{
    if (orientationReading->orientation() == QOrientationReading::TopDown
            || orientationReading->orientation() == QOrientationReading::FaceDown) {
        reset();
        return false;
    }
    return true;
}

void QTwistSensorGestureRecognizer::accelChanged(QAccelerometerReading *reading)
{
    if (orientationReading == 0)
        return;

    const qreal x = reading->x();
    const qreal y = reading->y();
    const qreal z = reading->z();

    if (!detecting && !checking&& dataList.count() > 21)
        dataList.removeFirst();

    qreal angle = qRadiansToDegrees(qAtan(x / qSqrt(y * y + z * z)));

    if (qAbs(angle) > 2) {
        if (detecting) {
            if ((angle > 0 && angle < lastAngle)
                    || (angle < 0 && angle > lastAngle)) {
                decreaseCount++;
            } else {
                if (decreaseCount > 0)
                decreaseCount--;
            }
        }

        if (!detecting && ((angle > 0 && angle > lastAngle)
                || (angle < 0 && angle < lastAngle))
                && ((angle > 0 && lastAngle > 0)
                    || (angle < 0 && lastAngle < 0))) {
            increaseCount++;
        } else
        if (!detecting && increaseCount > 3 && qAbs(angle) > 30) {
            decreaseCount = 0;
            detecting = true;
            detectedAngle = qRadiansToDegrees(qAtan(y / qSqrt(x * x + z * z)));
        }
    } else {
        increaseCount = 0;
        increaseCount = 0;
    }

    lastAngle = angle;
    if (detecting && decreaseCount >= 4 && qAbs(angle) < 25) {
        checkTwist();
    }

    twistAccelData data;
    data.x = x;
    data.y = y;
    data.z = z;

    if (qAbs(x) > 1)
        dataList.append(data);

    if (qAbs(z) > 15.0) {
        reset();
    }

}

void QTwistSensorGestureRecognizer::checkTwist()
{
    checking = true;
    int lastx = 0;
    bool ok = false;
    bool spinpoint = false;

    if (detectedAngle < 0) {
        reset();
        return;
    }

    //// check for orientation changes first
    if (orientationList.count() < 2)
        return;

    if (orientationList.count() > 2)
        if (orientationList.at(0) == orientationList.at(2)
                && (orientationList.at(1) == QOrientationReading::LeftUp
                    || orientationList.at(1) == QOrientationReading::RightUp)) {
            ok = true;
            if (orientationList.at(1) == QOrientationReading::RightUp)
                detectedAngle = 1;
            else
                detectedAngle = -1;
        }

    // now the manual increase/decrease count
    if (!ok) {
        if (increaseCount < 1 || decreaseCount < 3)
            return;

        if (increaseCount > 6 && decreaseCount > 4) {
            ok = true;
            if (orientationList.at(1) == QOrientationReading::RightUp)
                detectedAngle = 1;
            else
                detectedAngle = -1;
        }
    }
    // now we're really grasping for anything
    if (!ok)
        for (int i = 0; i < dataList.count(); i++) {
            twistAccelData curData = dataList.at(i);
            if (!spinpoint && qAbs(curData.x) < 1)
                continue;
            if (curData.z >= 0 ) {
                if (!spinpoint && (curData.x > lastx ||  curData.x < lastx) && curData.x - lastx  > 1) {
                    ok = true;
                } else if (spinpoint && (curData.x < lastx || curData.x > lastx)&& lastx - curData.x > 1) {
                    ok = true;
                } else {
                ok = false;
            }
        } else if (!spinpoint && curData.z < 0) {
            spinpoint = true;
        } else if (spinpoint && curData.z > 9) {
            break;
        }

        lastx = curData.x;
    }
    if (ok) {
        if (detectedAngle > 0) {
            Q_EMIT twistLeft();
            Q_EMIT detected("twistLeft");
        } else {
            Q_EMIT twistRight();
            Q_EMIT detected("twistRight");
        }
    }
    reset();
}

void QTwistSensorGestureRecognizer::reset()
{
    detecting = false;
    checking = false;
    dataList.clear();
    increaseCount = 0;
    decreaseCount = 0;
    lastAngle = 0;
}



QT_END_NAMESPACE
