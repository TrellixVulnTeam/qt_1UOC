// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qcapsuleshape_p.h"

#include <QtQuick3D/QQuick3DGeometry>
#include <geometry/PxCapsuleGeometry.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype CapsuleShape
    \inherits CollisionShape
    \inqmlmodule QtQuick3DPhysics
    \since 6.4
    \brief Capsule shape.

    This is the capsule shape. It is centered at the origin. It is specified by a diameter and a
    height value by which its axis extends along the positive and negative X-axis.

    \note When using a scale with this shape, the x component will be used to scale the height and
    the y component will be used to scale the diameter.
*/

/*!
    \qmlproperty float CapsuleShape::diameter
    This property defines the diameter of the capsule
*/

/*!
    \qmlproperty float CapsuleShape::height
    This property defines the height of the capsule
*/

QCapsuleShape::QCapsuleShape() = default;

QCapsuleShape::~QCapsuleShape()
{
    delete m_physXGeometry;
}

physx::PxGeometry *QCapsuleShape::getPhysXGeometry()
{
    if (!m_physXGeometry || m_scaleDirty) {
        updatePhysXGeometry();
    }

    return m_physXGeometry;
}

float QCapsuleShape::diameter() const
{
    return m_diameter;
}

void QCapsuleShape::setDiameter(float newDiameter)
{
    if (qFuzzyCompare(m_diameter, newDiameter))
        return;
    m_diameter = newDiameter;
    updatePhysXGeometry();

    emit needsRebuild(this);
    emit diameterChanged();
}

float QCapsuleShape::height() const
{
    return m_height;
}

void QCapsuleShape::setHeight(float newHeight)
{
    if (qFuzzyCompare(m_height, newHeight))
        return;
    m_height = newHeight;
    updatePhysXGeometry();

    emit needsRebuild(this);
    emit heightChanged();
}

void QCapsuleShape::updatePhysXGeometry()
{
    delete m_physXGeometry;
    QVector3D s = sceneScale();
    qreal rs = s.y();
    qreal hs = s.x();
    m_physXGeometry = new physx::PxCapsuleGeometry(rs * m_diameter * 0.5f, hs * m_height * 0.5f);
    m_scaleDirty = false;
}

QT_END_NAMESPACE
