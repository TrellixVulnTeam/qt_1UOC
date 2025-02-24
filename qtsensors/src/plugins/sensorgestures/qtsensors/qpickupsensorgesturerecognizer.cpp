// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpickupsensorgesturerecognizer.h"

#include <QtCore/qmath.h>

#define TIMER_TIMEOUT 250

QT_BEGIN_NAMESPACE

QPickupSensorGestureRecognizer::QPickupSensorGestureRecognizer(QObject *parent)
    : QSensorGestureRecognizer(parent)
    , accelReading(0)
    , active(0)
    , pXaxis(0)
    , pYaxis(0)
    , pZaxis(0)
    , lastpitch(0)
    , detecting(0)
{
}

QPickupSensorGestureRecognizer::~QPickupSensorGestureRecognizer()
{
}

void QPickupSensorGestureRecognizer::create()
{
}

QString QPickupSensorGestureRecognizer::id() const
{
    return QString("QtSensors.pickup");
}

bool QPickupSensorGestureRecognizer::start()
{
    if (QtSensorGestureSensorHandler::instance()->startSensor(QtSensorGestureSensorHandler::Accel)) {
            active = true;
            connect(QtSensorGestureSensorHandler::instance(),SIGNAL(accelReadingChanged(QAccelerometerReading*)),
                    this,SLOT(accelChanged(QAccelerometerReading*)));
        } else {
            QtSensorGestureSensorHandler::instance()->stopSensor(QtSensorGestureSensorHandler::Accel);
            active = false;
        }
    clear();

    return active;

}

bool QPickupSensorGestureRecognizer::stop()
{
    QtSensorGestureSensorHandler::instance()->stopSensor(QtSensorGestureSensorHandler::Accel);
    disconnect(QtSensorGestureSensorHandler::instance(),SIGNAL(accelReadingChanged(QAccelerometerReading*)),
            this,SLOT(accelChanged(QAccelerometerReading*)));
    active = false;

    return active;
}

bool QPickupSensorGestureRecognizer::isActive()
{
    return active;
}

#define PICKUP_BOTTOM_THRESHOLD 25
#define PICKUP_TOP_THRESHOLD 80
#define PICKUP_ANGLE_THRESHOLD 25
#define PICKUP_ROLL_THRESHOLD 13

void QPickupSensorGestureRecognizer::accelChanged(QAccelerometerReading *reading)
{
    accelReading = reading;
    const qreal x = reading->x();
    const qreal y = reading->y();
    const qreal z = reading->z();
    const qreal xdiff =  pXaxis - x;
    const qreal ydiff = pYaxis - y;
    const qreal zdiff =  pZaxis - z;

    qreal pitch = qRadiansToDegrees(qAtan(y / qSqrt(x * x + z * z)));
    qreal roll  = qRadiansToDegrees(qAtan(x / qSqrt(y * y + z * z)));

    if ((qAbs(xdiff) < 0.7 && qAbs(ydiff) < .7 && qAbs(zdiff) < .7)
            || z < 0) {
        detecting = false;
    } else if (pitch > PICKUP_BOTTOM_THRESHOLD && pitch < PICKUP_TOP_THRESHOLD) {
        detecting = true;
     }

    if ( pitchList.count() > 21) {
        pitchList.removeFirst();
    }
    if ( rollList.count() > 21) {
        rollList.removeFirst();
    }

    if (pitch > 1) {
        pitchList.append(pitch);
    }
    if (roll > 1) {
        rollList.append(roll);
    }

    if (detecting && pitchList.count() > 5 ) {
       timeout();
    }

    lastpitch = pitch;
    pXaxis = x;
    pYaxis = y;
    pZaxis = z;
}

void QPickupSensorGestureRecognizer::timeout()
{
    qreal averageRoll = 0;
    for (int r = 0; r < rollList.count(); r++) {
        averageRoll += rollList.at(r);
    }
    averageRoll /= rollList.count();

    if (averageRoll >  PICKUP_ROLL_THRESHOLD) {
        clear();
        return;
    }
    if (pitchList.isEmpty()
            || pitchList.at(0) > PICKUP_BOTTOM_THRESHOLD) {
        clear();
        return;
    }

    qreal previousPitch = 0;
    qreal startPitch = -1.0;
    int goodCount = 0;

    qreal averagePitch = 0;
    for (int i = 0; i < pitchList.count(); i++) {
        averagePitch += pitchList.at(i);
        if (previousPitch < pitchList.at(i)
                && qAbs(pitchList.at(i)) - qAbs(previousPitch) < 20) {
            if (goodCount == 1 && previousPitch != 0) {
                startPitch = previousPitch;
            }
            goodCount++;
        }

        previousPitch = pitchList.at(i);
    }
    averagePitch /= pitchList.count();

    if (averagePitch < 5) {
        clear();
        return;
    }

    if (goodCount >= 3 &&
            (pitchList.last() < PICKUP_TOP_THRESHOLD
             && pitchList.last() > PICKUP_BOTTOM_THRESHOLD)
            && startPitch > 0
            && (pitchList.last() - startPitch) > PICKUP_ANGLE_THRESHOLD) {
        Q_EMIT pickup();
        Q_EMIT detected("pickup");
    }
    clear();
}

void QPickupSensorGestureRecognizer::clear()
{
    pitchList.clear();
    detecting = false;
}

QT_END_NAMESPACE

