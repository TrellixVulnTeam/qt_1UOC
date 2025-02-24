// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderLayer::QSSGRenderLayer()
    : QSSGRenderNode(QSSGRenderNode::Type::Layer)
    , firstEffect(nullptr)
    , antialiasingMode(QSSGRenderLayer::AAMode::NoAA)
    , antialiasingQuality(QSSGRenderLayer::AAQuality::High)
    , background(QSSGRenderLayer::Background::Transparent)
    , horizontalFieldValues(QSSGRenderLayer::HorizontalField::LeftWidth)
    , m_left(0)
    , leftUnits(QSSGRenderLayer::UnitType::Percent)
    , m_width(100.0f)
    , widthUnits(QSSGRenderLayer::UnitType::Percent)
    , m_right(0)
    , rightUnits(QSSGRenderLayer::UnitType::Percent)
    , verticalFieldValues(QSSGRenderLayer::VerticalField::TopHeight)
    , m_top(0)
    , topUnits(QSSGRenderLayer::UnitType::Percent)
    , m_height(100.0f)
    , heightUnits(QSSGRenderLayer::UnitType::Percent)
    , m_bottom(0)
    , bottomUnits(QSSGRenderLayer::UnitType::Percent)
    , aoStrength(0)
    , aoDistance(5.0f)
    , aoSoftness(50.0f)
    , aoBias(0)
    , aoSamplerate(2)
    , aoDither(false)
    , lightProbe(nullptr)
    , probeExposure(1.0f)
    , probeHorizon(-1.0f)
    , temporalAAEnabled(false)
    , temporalAAStrength(0.3f)
    , ssaaEnabled(false)
    , ssaaMultiplier(1.5f)
    , specularAAEnabled(false)
    , explicitCamera(nullptr)
    , renderedCamera(nullptr)
    , tonemapMode(TonemapMode::Linear)
{
}

QSSGRenderLayer::~QSSGRenderLayer()
{
    if (importSceneNode) {
        // Remove the dummy from the list or it's siblings will still link to it.
        children.remove(*importSceneNode);
        importSceneNode->children.clear();
        delete importSceneNode;
        importSceneNode = nullptr;
    }
    delete renderData;
}

void QSSGRenderLayer::setProbeOrientation(const QVector3D &angles)
{
    if (angles != probeOrientationAngles) {
        probeOrientationAngles = angles;
        probeOrientation = QQuaternion::fromEulerAngles(probeOrientationAngles).toRotationMatrix();
    }
}

void QSSGRenderLayer::addEffect(QSSGRenderEffect &inEffect)
{
    // Effects need to be rendered in reverse order as described in the file.
    inEffect.m_nextEffect = firstEffect;
    firstEffect = &inEffect;
    inEffect.m_layer = this;
}

void QSSGRenderLayer::setImportScene(QSSGRenderNode &rootNode)
{
    // We create a dummy node to represent the imported scene tree, as we
    // do absolutely not want to change the node links in that tree!
    if (importSceneNode == nullptr) {
        importSceneNode = new QSSGRenderNode(QSSGRenderGraphObject::Type::ImportScene);
        // Now we can add the dummy node to the layers child list
        children.push_back(*importSceneNode);
    } else {
        importSceneNode->children.clear(); // Clear the list (or the list will modify the rootNode)
    }

    // The imported scene root node is now a child of the dummy node
    auto &importChildren = importSceneNode->children;
    Q_ASSERT(importChildren.isEmpty());
    // We don't want the list to modify our node, so we set the tail and head manually.
    importChildren.m_head = children.m_tail = &rootNode;
}

void QSSGRenderLayer::removeImportScene(QSSGRenderNode &rootNode)
{
    if (importSceneNode && !importSceneNode->children.isEmpty()) {
        if (&importSceneNode->children.back() == &rootNode)
            importSceneNode->children.clear();
    }
}

QSSGRenderEffect *QSSGRenderLayer::getLastEffect()
{
    if (firstEffect) {
        QSSGRenderEffect *theEffect = firstEffect;
        // Empty loop intentional
        for (; theEffect->m_nextEffect; theEffect = theEffect->m_nextEffect);
        Q_ASSERT(theEffect->m_nextEffect == nullptr);
        return theEffect;
    }
    return nullptr;
}

QT_END_NAMESPACE
