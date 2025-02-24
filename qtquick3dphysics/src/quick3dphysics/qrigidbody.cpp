// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qrigidbody_p.h"
#include "qphysicscommands_p.h"
#include "qdynamicsworld_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype DynamicRigidBody
    \inqmlmodule QtQuick3DPhysics
    \inherits PhysicsBody
    \since 6.4
    \brief Dynamic rigid body.

    This is the dynamic rigid body. A dynamic rigid body is an object that is part of the physics
    scene and behaves like a physical object with mass and velocity. Note that triangle mesh,
    heightfield and plane geometry shapes are only allowed as collision shapes when \l isKinematic
    is \c true.
*/

/*!
    \qmlproperty float DynamicRigidBody::mass

    This property defines the mass of the body. Note that this is only used when massMode is not
    \c {DynamicRigidBody.Density}. Also note that a value of 0 is interpreted as infinite mass
    and that negative numbers are not allowed.

    Default value is \c 1.

    \sa massMode
*/

/*!
    \qmlproperty float DynamicRigidBody::density

    This property defines the density of the body. This is only used when massMode is set to \c
    {DynamicRigidBody.Density}. When this property is less than or equal to zero, this body will
    use the \l {DynamicsWorld::}{defaultDensity} value.

    Default value is \c -1.
    \sa massMode
*/

/*!
    \qmlproperty vector3d DynamicRigidBody::linearVelocity
    This property defines the linear velocity of the body.
*/

/*!
    \qmlproperty vector3d DynamicRigidBody::angularVelocity
    This property defines the angular velocity of the body.
*/

/*!
    \qmlproperty bool DynamicRigidBody::axisLockLinearX
    This property locks the linear velocity of the body along the X-axis.
*/

/*!
    \qmlproperty bool DynamicRigidBody::axisLockLinearY
    This property locks the linear velocity of the body along the Y-axis.
*/

/*!
    \qmlproperty bool DynamicRigidBody::axisLockLinearZ
    This property locks the linear velocity of the body along the Z-axis.
*/

/*!
    \qmlproperty bool DynamicRigidBody::axisLockAngularX
    This property locks the angular velocity of the body along the X-axis.
*/

/*!
    \qmlproperty bool DynamicRigidBody::axisLockAngularY
    This property locks the angular velocity of the body along the Y-axis.
*/

/*!
    \qmlproperty bool DynamicRigidBody::axisLockAngularZ
    This property locks the angular velocity of the body along the Z-axis.
*/

/*!
    \qmlproperty bool DynamicRigidBody::isKinematic
    This property defines whether the object is kinematic or not. A kinematic object does not get
    influenced by external forces and can be seen as an object of infinite mass. If this property is
    set then in every simulation frame the physical object will be moved to its target position
    regardless of external forces.
*/

/*!
    \qmlproperty bool DynamicRigidBody::gravityEnabled
    This property defines whether the object is going to be affected by gravity or not.
*/

/*!
    \qmlproperty MassMode DynamicRigidBody::massMode

    This property holds the enum which describes how mass and inertia are calculated for this body.

    By default, \c DynamicRigidBody.Density is used.

    Available options:

    \value  DynamicRigidBody.Density
            Use the specified density to calculate mass and inertia assuming a uniform density.
            If density is non-positive then the \l {DynamicsWorld::}{defaultDensity} property in
            DynamicsWorld is used.

    \value  DynamicRigidBody.Mass
            Use the specified mass to calculate inertia assuming a uniform density.

    \value  DynamicRigidBody.MassAndInertiaTensor
            Use the specified mass value and inertia tensor.

    \value  DynamicRigidBody.MassAndInertiaMatrix
            Use the specified mass value and calculate inertia from the specified inertia
            matrix.
*/

/*!
    \qmlproperty vector3d DynamicRigidBody::inertiaTensor

    Defines the inertia tensor vector, using a parameter specified in mass space coordinates.

    This is the diagonal vector of a 3x3 diagonal matrix, if you have a non diagonal world/actor
    space inertia tensor then you should use \l{DynamicRigidBody::inertiaMatrix}{inertiaMatrix}
    instead.

    The inertia tensor components must be positive and a value of 0 in any component is
    interpreted as infinite inertia along that axis. Note that this is only used when
    massMode is set to \c DynamicRigidBody.MassAndInertiaTensor.

    Default value is (1, 1, 1).

    \sa massMode, inertiaMatrix
*/

/*!
    \qmlproperty vector3d DynamicRigidBody::centerOfMassPosition

    Defines the position of the center of mass relative to the body. Note that this is only used
    when massMode is set to \c DynamicRigidBody.MassAndInertiaTensor.

    \sa massMode, inertiaTensor
*/

/*!
    \qmlproperty quaternion DynamicRigidBody::centerOfMassRotation

    Defines the rotation of the center of mass pose, i.e. it specifies the orientation of the body's
    principal inertia axes relative to the body. Note that this is only used when massMode is set to
    \c DynamicRigidBody.MassAndInertiaTensor.

    \sa massMode, inertiaTensor
*/

/*!
    \qmlproperty list<float> DynamicRigidBody::inertiaMatrix

    Defines the inertia tensor matrix. This is a 3x3 matrix in column-major order. Note that this
    matrix is expected to be diagonalizable. Note that this is only used when massMode is set to
    \c DynamicRigidBody.MassAndInertiaMatrix.

    \sa massMode, inertiaTensor
*/

/*!
    \qmlmethod DynamicRigidBody::applyCentralForce(vector3d force)

    Applies  a \a force on the center of the body.
*/

/*!
    \qmlmethod DynamicRigidBody::applyForce(vector3d force, vector3d position)

    Applies a \a force at a \a position on the body.
*/

/*!
    \qmlmethod DynamicRigidBody::applyTorque(vector3d torque)

    Applies a \a torque on the body.
*/

/*!
    \qmlmethod DynamicRigidBody::applyCentralImpulse(vector3d impulse)

    Applies an \a impulse on the center of the body.
*/

/*!
    \qmlmethod DynamicRigidBody::applyImpulse(vector3d impulse, vector3d position)

    Applies an \a impulse at a \a position on the body.
*/

/*!
    \qmlmethod DynamicRigidBody::applyTorqueImpulse(vector3d impulse)

    Applies a torque \a impulse on the body.
*/

/*!
    \qmlmethod DynamicRigidBody::reset(vector3d position, vector3d eulerRotation)

    Resets the body's \a position and \a eulerRotation.
*/

QDynamicRigidBody::QDynamicRigidBody() = default;

QDynamicRigidBody::~QDynamicRigidBody()
{
    qDeleteAll(m_commandQueue);
    m_commandQueue.clear();
}

const QQuaternion &QDynamicRigidBody::centerOfMassRotation() const
{
    return m_centerOfMassRotation;
}

void QDynamicRigidBody::setCenterOfMassRotation(const QQuaternion &newCenterOfMassRotation)
{
    if (qFuzzyCompare(m_centerOfMassRotation, newCenterOfMassRotation))
        return;
    m_centerOfMassRotation = newCenterOfMassRotation;

    // Only inertia tensor is using rotation
    if (m_massMode == MassMode::MassAndInertiaTensor)
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaTensor(m_mass, m_inertiaTensor));

    emit centerOfMassRotationChanged();
}

const QVector3D &QDynamicRigidBody::centerOfMassPosition() const
{
    return m_centerOfMassPosition;
}

void QDynamicRigidBody::setCenterOfMassPosition(const QVector3D &newCenterOfMassPosition)
{
    if (qFuzzyCompare(m_centerOfMassPosition, newCenterOfMassPosition))
        return;

    switch (m_massMode) {
    case MassMode::MassAndInertiaTensor: {
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaTensor(m_mass, m_inertiaTensor));
        break;
    }
    case MassMode::MassAndInertiaMatrix: {
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaMatrix(m_mass, m_inertiaMatrix));
        break;
    }
    case MassMode::Density:
    case MassMode::Mass:
        break;
    }

    m_centerOfMassPosition = newCenterOfMassPosition;
    emit centerOfMassPositionChanged();
}

QDynamicRigidBody::MassMode QDynamicRigidBody::massMode() const
{
    return m_massMode;
}

void QDynamicRigidBody::setMassMode(const MassMode newMassMode)
{
    if (m_massMode == newMassMode)
        return;

    switch (newMassMode) {
    case MassMode::Density: {
        const float density =
                m_density < 0.f ? QDynamicsWorld::getWorld()->defaultDensity() : m_density;
        m_commandQueue.enqueue(new QPhysicsCommandSetDensity(density));
        break;
    }
    case MassMode::Mass: {
        m_commandQueue.enqueue(new QPhysicsCommandSetMass(m_mass));
        break;
    }
    case MassMode::MassAndInertiaTensor: {
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaTensor(m_mass, m_inertiaTensor));
        break;
    }
    case MassMode::MassAndInertiaMatrix: {
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaMatrix(m_mass, m_inertiaMatrix));
        break;
    }
    }

    m_massMode = newMassMode;
    emit massModeChanged();
}

const QVector3D &QDynamicRigidBody::inertiaTensor() const
{
    return m_inertiaTensor;
}

void QDynamicRigidBody::setInertiaTensor(const QVector3D &newInertiaTensor)
{
    if (qFuzzyCompare(m_inertiaTensor, newInertiaTensor))
        return;
    m_inertiaTensor = newInertiaTensor;

    if (m_massMode == MassMode::MassAndInertiaTensor)
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaTensor(m_mass, m_inertiaTensor));

    emit inertiaTensorChanged();
}

const QList<float> &QDynamicRigidBody::readInertiaMatrix() const
{
    return m_inertiaMatrixList;
}

static bool fuzzyEquals(const QList<float> &a, const QList<float> &b)
{
    if (a.length() != b.length())
        return false;

    const int length = a.length();
    for (int i = 0; i < length; i++)
        if (!qFuzzyCompare(a[i], b[i]))
            return false;

    return true;
}

void QDynamicRigidBody::setInertiaMatrix(const QList<float> &newInertiaMatrix)
{
    if (fuzzyEquals(m_inertiaMatrixList, newInertiaMatrix))
        return;

    m_inertiaMatrixList = newInertiaMatrix;
    const int elemsToCopy = qMin(m_inertiaMatrixList.length(), 9);
    memcpy(m_inertiaMatrix.data(), m_inertiaMatrixList.data(), elemsToCopy * sizeof(float));
    memset(m_inertiaMatrix.data() + elemsToCopy, 0, (9 - elemsToCopy) * sizeof(float));

    if (m_massMode == MassMode::MassAndInertiaMatrix)
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaMatrix(m_mass, m_inertiaMatrix));

    emit inertiaMatrixChanged();
}

const QMatrix3x3 &QDynamicRigidBody::inertiaMatrix() const
{
    return m_inertiaMatrix;
}

float QDynamicRigidBody::mass() const
{
    return m_mass;
}

QVector3D QDynamicRigidBody::linearVelocity() const
{
    return m_linearVelocity;
}

bool QDynamicRigidBody::isKinematic() const
{
    return m_isKinematic;
}

bool QDynamicRigidBody::gravityEnabled() const
{
    return m_gravityEnabled;
}

void QDynamicRigidBody::setMass(float mass)
{
    if (mass < 0.f || qFuzzyCompare(m_mass, mass))
        return;

    switch (m_massMode) {
    case QDynamicRigidBody::MassMode::Mass:
        m_commandQueue.enqueue(new QPhysicsCommandSetMass(mass));
        break;
    case QDynamicRigidBody::MassMode::MassAndInertiaTensor:
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaTensor(mass, m_inertiaTensor));
        break;
    case QDynamicRigidBody::MassMode::MassAndInertiaMatrix:
        m_commandQueue.enqueue(new QPhysicsCommandSetMassAndInertiaMatrix(mass, m_inertiaMatrix));
        break;
    case QDynamicRigidBody::MassMode::Density:
        break;
    }

    m_mass = mass;
    emit massChanged(m_mass);
}

float QDynamicRigidBody::density() const
{
    return m_density;
}

void QDynamicRigidBody::setDensity(float density)
{
    if (qFuzzyCompare(m_density, density))
        return;

    if (m_massMode == MassMode::Density && m_density > 0.f)
        m_commandQueue.enqueue(new QPhysicsCommandSetDensity(density));

    m_density = density;
    emit densityChanged(m_density);
}

void QDynamicRigidBody::setLinearVelocity(QVector3D linearVelocity)
{
    if (m_linearVelocity == linearVelocity)
        return;

    m_linearVelocity = linearVelocity;
    m_commandQueue.enqueue(new QPhysicsCommandSetLinearVelocity(m_linearVelocity));
    emit linearVelocityChanged(m_linearVelocity);
}

void QDynamicRigidBody::setIsKinematic(bool isKinematic)
{
    if (m_isKinematic == isKinematic)
        return;

    if (hasStaticShapes() && !isKinematic) {
        qWarning()
                << "Cannot make body containing trimesh/heightfield/plane non-kinematic, ignoring.";
        return;
    }

    m_isKinematic = isKinematic;
    m_commandQueue.enqueue(new QPhysicsCommandSetIsKinematic(m_isKinematic));
    emit isKinematicChanged(m_isKinematic);
}

void QDynamicRigidBody::setGravityEnabled(bool gravityEnabled)
{
    if (m_gravityEnabled == gravityEnabled)
        return;

    m_gravityEnabled = gravityEnabled;
    m_commandQueue.enqueue(new QPhysicsCommandSetGravityEnabled(m_gravityEnabled));
    emit gravityEnabledChanged();
}

const QVector3D &QDynamicRigidBody::angularVelocity() const
{
    return m_angularVelocity;
}

void QDynamicRigidBody::setAngularVelocity(const QVector3D &newAngularVelocity)
{
    if (m_angularVelocity == newAngularVelocity)
        return;
    m_angularVelocity = newAngularVelocity;
    m_commandQueue.enqueue(new QPhysicsCommandSetAngularVelocity(m_angularVelocity));
    emit angularVelocityChanged();
}

bool QDynamicRigidBody::axisLockLinearX() const
{
    return m_axisLockLinearX;
}

void QDynamicRigidBody::setAxisLockLinearX(bool newAxisLockLinearX)
{
    if (m_axisLockLinearX == newAxisLockLinearX)
        return;
    m_axisLockLinearX = newAxisLockLinearX;
    emit axisLockLinearXChanged();
}

bool QDynamicRigidBody::axisLockLinearY() const
{
    return m_axisLockLinearY;
}

void QDynamicRigidBody::setAxisLockLinearY(bool newAxisLockLinearY)
{
    if (m_axisLockLinearY == newAxisLockLinearY)
        return;
    m_axisLockLinearY = newAxisLockLinearY;
    emit axisLockLinearYChanged();
}

bool QDynamicRigidBody::axisLockLinearZ() const
{
    return m_axisLockLinearZ;
}

void QDynamicRigidBody::setAxisLockLinearZ(bool newAxisLockLinearZ)
{
    if (m_axisLockLinearZ == newAxisLockLinearZ)
        return;
    m_axisLockLinearZ = newAxisLockLinearZ;
    emit axisLockLinearZChanged();
}

bool QDynamicRigidBody::axisLockAngularX() const
{
    return m_axisLockAngularX;
}

void QDynamicRigidBody::setAxisLockAngularX(bool newAxisLockAngularX)
{
    if (m_axisLockAngularX == newAxisLockAngularX)
        return;
    m_axisLockAngularX = newAxisLockAngularX;
    emit axisLockAngularXChanged();
}

bool QDynamicRigidBody::axisLockAngularY() const
{
    return m_axisLockAngularY;
}

void QDynamicRigidBody::setAxisLockAngularY(bool newAxisLockAngularY)
{
    if (m_axisLockAngularY == newAxisLockAngularY)
        return;
    m_axisLockAngularY = newAxisLockAngularY;
    emit axisLockAngularYChanged();
}

bool QDynamicRigidBody::axisLockAngularZ() const
{
    return m_axisLockAngularZ;
}

void QDynamicRigidBody::setAxisLockAngularZ(bool newAxisLockAngularZ)
{
    if (m_axisLockAngularZ == newAxisLockAngularZ)
        return;
    m_axisLockAngularZ = newAxisLockAngularZ;
    emit axisLockAngularZChanged();
}

QQueue<QPhysicsCommand *> &QDynamicRigidBody::commandQueue()
{
    return m_commandQueue;
}

void QDynamicRigidBody::updateDefaultDensity(float defaultDensity)
{
    if (m_massMode == MassMode::Density && m_density <= 0.f)
        m_commandQueue.enqueue(new QPhysicsCommandSetDensity(defaultDensity));
}

void QDynamicRigidBody::applyCentralForce(const QVector3D &force)
{
    m_commandQueue.enqueue(new QPhysicsCommandApplyCentralForce(force));
}

void QDynamicRigidBody::applyForce(const QVector3D &force, const QVector3D &position)
{
    m_commandQueue.enqueue(new QPhysicsCommandApplyForce(force, position));
}

void QDynamicRigidBody::applyTorque(const QVector3D &torque)
{
    m_commandQueue.enqueue(new QPhysicsCommandApplyTorque(torque));
}

void QDynamicRigidBody::applyCentralImpulse(const QVector3D &impulse)
{
    m_commandQueue.enqueue(new QPhysicsCommandApplyCentralImpulse(impulse));
}

void QDynamicRigidBody::applyImpulse(const QVector3D &impulse, const QVector3D &position)
{
    m_commandQueue.enqueue(new QPhysicsCommandApplyImpulse(impulse, position));
}

void QDynamicRigidBody::applyTorqueImpulse(const QVector3D &impulse)
{
    m_commandQueue.enqueue(new QPhysicsCommandApplyTorqueImpulse(impulse));
}

void QDynamicRigidBody::reset(const QVector3D &position, const QVector3D &eulerRotation)
{
    m_commandQueue.enqueue(new QPhysicsCommandReset(position, eulerRotation));
}

/*!
    \qmltype StaticRigidBody
    \inqmlmodule QtQuick3DPhysics
    \inherits PhysicsBody
    \since 6.4
    \brief Static rigid body.

    This is an immovable and static rigid body. It is technically possible to move the body but it
    will incur a performance penalty. Any collision shape is allowed for this body.
*/

QStaticRigidBody::QStaticRigidBody() = default;

QT_END_NAMESPACE
