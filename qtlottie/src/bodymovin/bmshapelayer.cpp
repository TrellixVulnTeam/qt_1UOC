// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmshapelayer_p.h"

#include <QJsonObject>
#include <QJsonArray>


#include "bmconstants_p.h"
#include "bmbase_p.h"
#include "bmshape_p.h"
#include "bmtrimpath_p.h"
#include "bmbasictransform_p.h"
#include "lottierenderer_p.h"

QT_BEGIN_NAMESPACE

BMShapeLayer::BMShapeLayer(const BMShapeLayer &other)
    : BMLayer(other)
{
    m_maskProperties = other.m_maskProperties;
    m_layerTransform = new BMBasicTransform(*other.m_layerTransform);
    m_layerTransform->setParent(this);
    m_appliedTrim = other.m_appliedTrim;
}

BMShapeLayer::BMShapeLayer(const QJsonObject &definition)
{
    m_type = BM_LAYER_SHAPE_IX;

    BMLayer::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMShapeLayer::BMShapeLayer()"
                                       << m_name;

    QJsonArray maskProps = definition.value(QLatin1String("maskProperties")).toArray();
    QJsonArray::const_iterator propIt = maskProps.constBegin();
    while (propIt != maskProps.constEnd()) {
        m_maskProperties.append((*propIt).toVariant().toInt());
        ++propIt;
    }

    QJsonObject trans = definition.value(QLatin1String("ks")).toObject();
    m_layerTransform = new BMBasicTransform(trans, this);

    QJsonArray items = definition.value(QLatin1String("shapes")).toArray();
    QJsonArray::const_iterator itemIt = items.constEnd();
    while (itemIt != items.constBegin()) {
        itemIt--;
        BMShape *shape = BMShape::construct((*itemIt).toObject(), this);
        if (shape)
            appendChild(shape);
    }

    if (m_maskProperties.length())
        qCWarning(lcLottieQtBodymovinParser)
            << "BM Shape Layer: mask properties found, but not supported"
            << m_maskProperties;
}

BMShapeLayer::~BMShapeLayer()
{
    if (m_layerTransform)
        delete m_layerTransform;
}

BMBase *BMShapeLayer::clone() const
{
    return new BMShapeLayer(*this);
}

void BMShapeLayer::updateProperties(int frame)
{
    BMLayer::updateProperties(frame);

    m_layerTransform->updateProperties(frame);

    for (BMBase *child : children()) {
        if (child->hidden())
            continue;

        BMShape *shape = dynamic_cast<BMShape*>(child);

        if (!shape)
            continue;

        if (shape->type() == BM_SHAPE_TRIM_IX) {
            BMTrimPath *trim = static_cast<BMTrimPath*>(shape);
            if (m_appliedTrim)
                m_appliedTrim->applyTrim(*trim);
            else
                m_appliedTrim = trim;
        } else if (m_appliedTrim) {
            if (shape->acceptsTrim())
                shape->applyTrim(*m_appliedTrim);
        }
    }
}

void BMShapeLayer::render(LottieRenderer &renderer) const
{
    renderer.saveState();

    renderEffects(renderer);

    // In case there is a linked layer, apply its transform first
    // as it affects tranforms of this layer too
    if (BMLayer *ll = linkedLayer())
        renderer.render(*ll->transform());

    renderer.render(*this);

    m_layerTransform->render(renderer);

    for (BMBase *child : children()) {
        if (child->hidden())
            continue;
        child->render(renderer);
    }

    if (m_appliedTrim && !m_appliedTrim->hidden())
        m_appliedTrim->render(renderer);

    renderer.restoreState();
}

QT_END_NAMESPACE
