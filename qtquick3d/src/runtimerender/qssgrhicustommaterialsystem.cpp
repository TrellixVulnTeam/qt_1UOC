// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrhicustommaterialsystem_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiparticles_p.h>

#include <QtCore/qbitarray.h>

QT_BEGIN_NAMESPACE

QSSGCustomMaterialSystem::QSSGCustomMaterialSystem() = default;

QSSGCustomMaterialSystem::~QSSGCustomMaterialSystem()
{
}

bool QSSGCustomMaterialSystem::prepareForRender(const QSSGRenderModel &,
                                          const QSSGRenderSubset &,
                                          QSSGRenderCustomMaterial &inMaterial)
{
    return inMaterial.isDirty();
}

void QSSGCustomMaterialSystem::setRenderContextInterface(QSSGRenderContextInterface *inContext)
{
    context = inContext;
}

QSSGRef<QSSGRhiShaderPipeline> QSSGCustomMaterialSystem::shadersForCustomMaterial(QSSGRhiGraphicsPipelineState *ps,
                                                                                  const QSSGRenderCustomMaterial &material,
                                                                                  QSSGSubsetRenderable &renderable,
                                                                                  const QSSGShaderFeatures &featureSet)
{
    // This just references inFeatureSet and inRenderable.shaderDescription -
    // cheap to construct and is good enough for the find()
    QSSGShaderMapKey skey = QSSGShaderMapKey(material.m_shaderPathKey,
                                             featureSet,
                                             renderable.shaderDescription);

    QSSGRef<QSSGRhiShaderPipeline> shaderPipeline;
    auto it = shaderMap.find(skey);
    if (it == shaderMap.end()) {
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DGenerateShader);
        QSSGMaterialVertexPipeline pipeline(context->shaderProgramGenerator(),
                                            context->renderer()->defaultMaterialShaderKeyProperties(),
                                            material.adapter);

        shaderPipeline = QSSGMaterialShaderGenerator::generateMaterialRhiShader(material.m_shaderPathKey,
                                                                                pipeline,
                                                                                renderable.shaderDescription,
                                                                                context->renderer()->defaultMaterialShaderKeyProperties(),
                                                                                featureSet,
                                                                                renderable.material,
                                                                                renderable.lights,
                                                                                renderable.firstImage,
                                                                                context->shaderLibraryManager(),
                                                                                context->shaderCache());
        Q_QUICK3D_PROFILE_END(QQuick3DProfiler::Quick3DGenerateShader);

        // make skey useable as a key for the QHash (makes copies of materialKey and featureSet, instead of just referencing)
        skey.detach();
        // insert it no matter what, no point in trying over and over again
        shaderMap.insert(skey, shaderPipeline);
    } else {
        shaderPipeline = it.value();
    }

    if (shaderPipeline) {
        ps->shaderPipeline = shaderPipeline.data();
        shaderPipeline->resetExtraTextures();
    }

    return shaderPipeline;
}

void QSSGCustomMaterialSystem::updateUniformsForCustomMaterial(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                                               QSSGRhiContext *rhiCtx,
                                                               char *ubufData,
                                                               QSSGRhiGraphicsPipelineState *ps,
                                                               const QSSGRenderCustomMaterial &material,
                                                               QSSGSubsetRenderable &renderable,
                                                               QSSGLayerRenderData &layerData,
                                                               QSSGRenderCamera &camera,
                                                               const QVector2D *depthAdjust,
                                                               const QMatrix4x4 *alteredModelViewProjection)
{
    const QMatrix4x4 &mvp(alteredModelViewProjection ? *alteredModelViewProjection
                                                     : renderable.modelContext.modelViewProjection);

    const QMatrix4x4 clipSpaceCorrMatrix = rhiCtx->rhi()->clipSpaceCorrMatrix();
    QRhi *rhi = rhiCtx->rhi();

    const QSSGLayerGlobalRenderProperties globalProperties =
    {
        layerData.layer,
        camera,
        layerData.cameraDirection,
        layerData.shadowMapManager,
        layerData.rhiDepthTexture.texture,
        layerData.rhiAoTexture.texture,
        layerData.rhiScreenTexture.texture,
        layerData.layer.lightProbe,
        layerData.layer.probeHorizon,
        layerData.layer.probeExposure,
        layerData.layer.probeOrientation,
        rhi->isYUpInFramebuffer(),
        rhi->isYUpInNDC(),
        rhi->isClipDepthZeroToOne()
    };

    const auto &modelNode = renderable.modelContext.model;
    const QMatrix4x4 &localInstanceTransform(modelNode.localInstanceTransform);
    const QMatrix4x4 &globalInstanceTransform(modelNode.globalInstanceTransform);
    const QMatrix4x4 &modelMatrix((modelNode.boneCount == 0) ? renderable.globalTransform : QMatrix4x4());

    QSSGMaterialShaderGenerator::setRhiMaterialProperties(*context,
                                                          shaderPipeline,
                                                          ubufData,
                                                          ps,
                                                          material,
                                                          renderable.shaderDescription,
                                                          context->renderer()->defaultMaterialShaderKeyProperties(),
                                                          camera,
                                                          mvp,
                                                          renderable.modelContext.normalMatrix,
                                                          modelMatrix,
                                                          clipSpaceCorrMatrix,
                                                          localInstanceTransform,
                                                          globalInstanceTransform,
                                                          toDataView(modelNode.morphWeights),
                                                          renderable.firstImage,
                                                          renderable.opacity,
                                                          globalProperties,
                                                          renderable.lights,
                                                          renderable.reflectionProbe,
                                                          true,
                                                          renderable.renderableFlags.receivesReflections(),
                                                          depthAdjust,
                                                          renderable.modelContext.lightmapTexture);
}

static const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

void QSSGCustomMaterialSystem::rhiPrepareRenderable(QSSGRhiGraphicsPipelineState *ps,
                                                    QSSGSubsetRenderable &renderable,
                                                    const QSSGShaderFeatures &featureSet,
                                                    const QSSGRenderCustomMaterial &material,
                                                    QSSGLayerRenderData &layerData,
                                                    QRhiRenderPassDescriptor *renderPassDescriptor,
                                                    int samples,
                                                    QSSGRenderCamera *camera,
                                                    int cubeFace,
                                                    QMatrix4x4 *modelViewProjection,
                                                    QSSGReflectionMapEntry *entry)
{
    QSSGRhiContext *rhiCtx = context->rhiContext().data();

    QRhiGraphicsPipeline::TargetBlend blend; // no blending by default
    if (material.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Blending)) {
        blend.enable = true;
        blend.srcColor = material.m_srcBlend;
        blend.srcAlpha = material.m_srcBlend;
        blend.dstColor = material.m_dstBlend;
        blend.dstAlpha = material.m_dstBlend;
    }

    const QSSGCullFaceMode cullMode = material.m_cullMode;

    const bool blendParticles = renderable.generator->contextInterface()->renderer()->defaultMaterialShaderKeyProperties().m_blendParticles.getValue(renderable.shaderDescription);

    QSSGRef<QSSGRhiShaderPipeline> shaderPipeline = shadersForCustomMaterial(ps, material, renderable, featureSet);

    if (shaderPipeline) {
        QSSGRhiShaderResourceBindingList bindings;
        const auto &modelNode = renderable.modelContext.model;

        QSSGRhiDrawCallData &dcd(cubeFace < 0 ? rhiCtx->drawCallData({ &layerData.layer,
                                                        &modelNode,
                                                        &material,
                                                        0,
                                                        QSSGRhiDrawCallDataKey::Main })
                                              : rhiCtx->drawCallData({ &layerData.layer,
                                                                       &modelNode,
                                                                       entry, cubeFace + int(renderable.subset.offset << 3),
                                                                       QSSGRhiDrawCallDataKey::Reflection }));

        shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd.ubuf);
        char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
        if (!camera)
            updateUniformsForCustomMaterial(shaderPipeline, rhiCtx, ubufData, ps, material, renderable, layerData, *layerData.camera, nullptr, nullptr);
        else
            updateUniformsForCustomMaterial(shaderPipeline, rhiCtx, ubufData, ps, material, renderable, layerData, *camera, nullptr, modelViewProjection);
        if (blendParticles)
            QSSGParticleRenderer::updateUniformsForParticleModel(shaderPipeline, ubufData, &renderable.modelContext.model, renderable.subset.offset);
        dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

        if (blendParticles)
            QSSGParticleRenderer::prepareParticlesForModel(shaderPipeline, rhiCtx, bindings, &renderable.modelContext.model);
        bool instancing = false;
        if (!camera)
            instancing = renderable.prepareInstancing(rhiCtx, layerData.cameraDirection);
        else
            instancing = renderable.prepareInstancing(rhiCtx, camera->getScalingCorrectDirection());

        ps->samples = samples;

        ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(cullMode);

        ps->targetBlend = blend;

        ps->ia = renderable.subset.rhi.ia;

        //### Copied code from default materials
        int instanceBufferBinding = 0;
        if (instancing) {
            // Need to setup new bindings for instanced buffers
            const quint32 stride = renderable.modelContext.model.instanceTable->stride();
            QVarLengthArray<QRhiVertexInputBinding, 8> bindings;
            std::copy(ps->ia.inputLayout.cbeginBindings(),
                      ps->ia.inputLayout.cendBindings(),
                      std::back_inserter(bindings));
            bindings.append({ stride, QRhiVertexInputBinding::PerInstance });
            instanceBufferBinding = bindings.count() - 1;
            ps->ia.inputLayout.setBindings(bindings.cbegin(), bindings.cend());
        }

        ps->ia.bakeVertexInputLocations(*shaderPipeline, instanceBufferBinding);

        QRhiResourceUpdateBatch *resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();
        QRhiTexture *dummyTexture = rhiCtx->dummyTexture({}, resourceUpdates);
        QRhiTexture *dummyCubeTexture = rhiCtx->dummyTexture(QRhiTexture::CubeMap, resourceUpdates);
        rhiCtx->commandBuffer()->resourceUpdate(resourceUpdates);

        bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd.ubuf, 0, shaderPipeline->ub0Size());
        bindings.addUniformBuffer(1, VISIBILITY_ALL, dcd.ubuf,
                                  shaderPipeline->ub0LightDataOffset(),
                                  shaderPipeline->ub0LightDataSize());

        QVector<QShaderDescription::InOutVariable> samplerVars =
                shaderPipeline->fragmentStage()->shader().description().combinedImageSamplers();
        for (const QShaderDescription::InOutVariable &var : shaderPipeline->vertexStage()->shader().description().combinedImageSamplers()) {
            auto it = std::find_if(samplerVars.cbegin(), samplerVars.cend(),
                                   [&var](const QShaderDescription::InOutVariable &v) { return var.binding == v.binding; });
            if (it == samplerVars.cend())
                samplerVars.append(var);
        }

        int maxSamplerBinding = -1;
        for (const QShaderDescription::InOutVariable &var : samplerVars)
            maxSamplerBinding = qMax(maxSamplerBinding, var.binding);

        // Will need to set unused image-samplers to something dummy
        // because the shader code contains all custom property textures,
        // and not providing a binding for all of them is invalid with some
        // graphics APIs (and will need a real texture because setting a
        // null handle or similar is not permitted with some of them so the
        // srb does not accept null QRhiTextures either; but first let's
        // figure out what bindings are unused in this frame)
        QBitArray samplerBindingsSpecified(maxSamplerBinding + 1);

        if (blendParticles)
            samplerBindingsSpecified.setBit(shaderPipeline->bindingForTexture("qt_particleTexture"));

        // Skinning
        if (modelNode.boneCount != 0) {
            QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
            QRhiTextureSubresourceUploadDescription boneDesc(modelNode.boneData);
            QRhiTextureUploadDescription boneUploadDesc(QRhiTextureUploadEntry(0, 0, boneDesc));
            rub->uploadTexture(modelNode.boneTexture, boneUploadDesc);
            rhiCtx->commandBuffer()->resourceUpdate(rub);
            int binding = shaderPipeline->bindingForTexture("qt_boneTexture");
            if (binding >= 0) {
                QRhiSampler *boneSampler = rhiCtx->sampler({ QRhiSampler::Nearest,
                                                             QRhiSampler::Nearest,
                                                             QRhiSampler::None,
                                                             QRhiSampler::ClampToEdge,
                                                             QRhiSampler::ClampToEdge,
                                                             QRhiSampler::Repeat
                                                           });
                bindings.addTexture(binding, QRhiShaderResourceBinding::VertexStage, modelNode.boneTexture, boneSampler);
                samplerBindingsSpecified.setBit(binding);
            }
        }

        // Prioritize reflection texture over Light Probe texture because
        // reflection texture also contains the irradiance and pre filtered
        // values for the light probe.
        if (featureSet.isSet(QSSGShaderFeatures::Feature::ReflectionProbe)) {
            int reflectionSampler = shaderPipeline->bindingForTexture("qt_reflectionMap");
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
            QRhiTexture* reflectionTexture = layerData.reflectionMapManager->reflectionMapEntry(renderable.reflectionProbeIndex)->m_rhiPrefilteredCube;
            if (reflectionSampler >= 0 && reflectionTexture) {
                bindings.addTexture(reflectionSampler, QRhiShaderResourceBinding::FragmentStage, reflectionTexture, sampler);
                samplerBindingsSpecified.setBit(reflectionSampler);
            }
        } else if (shaderPipeline->lightProbeTexture()) {
            int binding = shaderPipeline->bindingForTexture("qt_lightProbe", int(QSSGRhiSamplerBindingHints::LightProbe));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                QPair<QSSGRenderTextureCoordOp, QSSGRenderTextureCoordOp> tiling = shaderPipeline->lightProbeTiling();
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, // enables mipmapping
                                                         toRhi(tiling.first), toRhi(tiling.second), QRhiSampler::Repeat });
                bindings.addTexture(binding,
                                    QRhiShaderResourceBinding::FragmentStage,
                                    shaderPipeline->lightProbeTexture(), sampler);
            } // else ignore, not an error (for example, an unshaded material's fragment shader will not have this sampler)
        }

        if (shaderPipeline->screenTexture()) {
            int binding = shaderPipeline->bindingForTexture("qt_screenTexture", int(QSSGRhiSamplerBindingHints::ScreenTexture));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                // linear min/mag, mipmap filtering depends on the
                // texture, with SCREEN_TEXTURE there are no mipmaps, but
                // once SCREEN_MIP_TEXTURE is seen the texture (the same
                // one) has mipmaps generated.
                QRhiSampler::Filter mipFilter = shaderPipeline->screenTexture()->flags().testFlag(QRhiTexture::MipMapped)
                        ? QRhiSampler::Linear : QRhiSampler::None;
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, mipFilter,
                                                         QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat });
                bindings.addTexture(binding,
                                    QRhiShaderResourceBinding::FragmentStage,
                                    shaderPipeline->screenTexture(), sampler);
            } // else ignore, not an error
        }

        if (shaderPipeline->depthTexture()) {
            int binding = shaderPipeline->bindingForTexture("qt_depthTexture", int(QSSGRhiSamplerBindingHints::DepthTexture));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                // nearest min/mag, no mipmap
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                bindings.addTexture(binding,
                                    QRhiShaderResourceBinding::FragmentStage,
                                    shaderPipeline->depthTexture(), sampler);
            } // else ignore, not an error
        }

        if (shaderPipeline->ssaoTexture()) {
            int binding = shaderPipeline->bindingForTexture("qt_aoTexture", int(QSSGRhiSamplerBindingHints::AoTexture));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                // linear min/mag, no mipmap
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                bindings.addTexture(binding,
                                    QRhiShaderResourceBinding::FragmentStage,
                                    shaderPipeline->ssaoTexture(), sampler);
            } // else ignore, not an error
        }

        if (shaderPipeline->lightmapTexture()) {
            int binding = shaderPipeline->bindingForTexture("qt_lightmap", int(QSSGRhiSamplerBindingHints::LightmapTexture));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                bindings.addTexture(binding,
                                    QRhiShaderResourceBinding::FragmentStage,
                                    shaderPipeline->lightmapTexture(), sampler);
            } // else ignore, not an error
        }

        const int shadowMapCount = shaderPipeline->shadowMapCount();
        for (int i = 0; i < shadowMapCount; ++i) {
            QSSGRhiShadowMapProperties &shadowMapProperties(shaderPipeline->shadowMapAt(i));
            QRhiTexture *texture = shadowMapProperties.shadowMapTexture;
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
            const QByteArray &name(shadowMapProperties.shadowMapTextureUniformName);
            if (shadowMapProperties.cachedBinding < 0)
                shadowMapProperties.cachedBinding = shaderPipeline->bindingForTexture(name);
            if (shadowMapProperties.cachedBinding < 0) // may not be used in the shader with unshaded custom materials, that's normal
                continue;
            samplerBindingsSpecified.setBit(shadowMapProperties.cachedBinding);
            bindings.addTexture(shadowMapProperties.cachedBinding,
                                QRhiShaderResourceBinding::FragmentStage,
                                texture,
                                sampler);
        }

        QSSGRenderableImage *renderableImage = renderable.firstImage;
        while (renderableImage) {
            const char *samplerName = QSSGMaterialShaderGenerator::getSamplerName(renderableImage->m_mapType);
            const int samplerHint = int(renderableImage->m_mapType);
            int samplerBinding = shaderPipeline->bindingForTexture(samplerName, samplerHint);
            if (samplerBinding >= 0) {
                QRhiTexture *texture = renderableImage->m_texture.m_texture;
                if (samplerBinding >= 0 && texture) {
                    const bool mipmapped = texture->flags().testFlag(QRhiTexture::MipMapped);
                    QRhiSampler *sampler = rhiCtx->sampler({ toRhi(renderableImage->m_imageNode.m_minFilterType),
                                                             toRhi(renderableImage->m_imageNode.m_magFilterType),
                                                             mipmapped ? toRhi(renderableImage->m_imageNode.m_mipFilterType) : QRhiSampler::None,
                                                             toRhi(renderableImage->m_imageNode.m_horizontalTilingMode),
                                                             toRhi(renderableImage->m_imageNode.m_verticalTilingMode),
                                                             QRhiSampler::Repeat
                                                           });
                    samplerBindingsSpecified.setBit(samplerBinding);
                    bindings.addTexture(samplerBinding,
                                        VISIBILITY_ALL,
                                        texture, sampler);
                }
            } // else this is not necessarily an error, e.g. having metalness/roughness maps with metalness disabled
            renderableImage = renderableImage->m_nextImage;
        }

        if (maxSamplerBinding >= 0) {
            // custom property textures
            int customTexCount = shaderPipeline->extraTextureCount();
            for (int i = 0; i < customTexCount; ++i) {
                const QSSGRhiTexture &t(shaderPipeline->extraTextureAt(i));
                const int samplerBinding = shaderPipeline->bindingForTexture(t.name);
                if (samplerBinding >= 0) {
                    samplerBindingsSpecified.setBit(samplerBinding);
                    QRhiSampler *sampler = rhiCtx->sampler(t.samplerDesc);
                    bindings.addTexture(samplerBinding,
                                        VISIBILITY_ALL,
                                        t.texture,
                                        sampler);
                }
            }

            // use a dummy texture for the unused samplers in the shader
            QRhiSampler *dummySampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });

            for (const QShaderDescription::InOutVariable &var : samplerVars) {
                if (!samplerBindingsSpecified.testBit(var.binding)) {
                    QRhiTexture *t = var.type == QShaderDescription::SamplerCube ? dummyCubeTexture : dummyTexture;
                    bindings.addTexture(var.binding, VISIBILITY_ALL, t, dummySampler);
                }
            }
        }

        // do the same srb lookup acceleration as default materials
        QRhiShaderResourceBindings *&srb = dcd.srb;
        bool srbChanged = false;
        if (!srb || bindings != dcd.bindings) {
            srb = rhiCtx->srb(bindings);
            dcd.bindings = bindings;
            srbChanged = true;
        }

        if (cubeFace < 0)
            renderable.rhiRenderData.mainPass.srb = srb;
        else
            renderable.rhiRenderData.reflectionPass.srb[cubeFace] = srb;

        const QSSGGraphicsPipelineStateKey pipelineKey = QSSGGraphicsPipelineStateKey::create(*ps, renderPassDescriptor, srb);
        if (dcd.pipeline
                && !srbChanged
                && dcd.renderTargetDescriptionHash == pipelineKey.extra.renderTargetDescriptionHash
                && dcd.renderTargetDescription == pipelineKey.renderTargetDescription
                && dcd.ps == *ps)
        {
            if (cubeFace < 0)
                renderable.rhiRenderData.mainPass.pipeline = dcd.pipeline;
            else
                renderable.rhiRenderData.reflectionPass.pipeline = dcd.pipeline;
        } else {
            if (cubeFace < 0) {
                renderable.rhiRenderData.mainPass.pipeline = rhiCtx->pipeline(pipelineKey,
                                                                              renderPassDescriptor,
                                                                              srb);
                dcd.pipeline = renderable.rhiRenderData.mainPass.pipeline;
            } else {
                renderable.rhiRenderData.reflectionPass.pipeline = rhiCtx->pipeline(pipelineKey,
                                                                                    renderPassDescriptor,
                                                                                    srb);
                dcd.pipeline = renderable.rhiRenderData.reflectionPass.pipeline;
            }

            dcd.renderTargetDescriptionHash = pipelineKey.extra.renderTargetDescriptionHash;
            dcd.renderTargetDescription = pipelineKey.renderTargetDescription;
            dcd.ps = *ps;
        }
    }
}

void QSSGCustomMaterialSystem::setShaderResources(char *ubufData,
                                                  const QSSGRenderCustomMaterial &inMaterial,
                                                  const QByteArray &inPropertyName,
                                                  const QVariant &propertyValue,
                                                  QSSGRenderShaderDataType inPropertyType,
                                                  const QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline)
{
    Q_UNUSED(inMaterial);

    if (inPropertyType == QSSGRenderShaderDataType::Texture) {
        QSSGRenderCustomMaterial::TextureProperty *textureProperty =
                reinterpret_cast<QSSGRenderCustomMaterial::TextureProperty *>(propertyValue.value<void *>());
        QSSGRenderImage *image = textureProperty->texImage;
        if (image) {
            const QSSGRef<QSSGBufferManager> &theBufferManager(context->bufferManager());

            QSSGBufferManager::MipMode mipMode = QSSGBufferManager::MipModeNone;
            // the mipFilterType here is only non-None when generateMipmaps was true on the Texture
            if (textureProperty->mipFilterType != QSSGRenderTextureFilterOp::None)
                mipMode = QSSGBufferManager::MipModeGenerated;
            // ### would we want MipModeBsdf in some cases?

            const QSSGRenderImageTexture texture = theBufferManager->loadRenderImage(image, mipMode);
            if (texture.m_texture) {
                const QSSGRhiTexture t = {
                    inPropertyName,
                    texture.m_texture,
                    { toRhi(textureProperty->minFilterType),
                      toRhi(textureProperty->magFilterType),
                      textureProperty->mipFilterType != QSSGRenderTextureFilterOp::None ? toRhi(textureProperty->mipFilterType) : QRhiSampler::None,
                      toRhi(textureProperty->horizontalClampType),
                      toRhi(textureProperty->verticalClampType),
                      QRhiSampler::Repeat
                    }
                };
                shaderPipeline->addExtraTexture(t);
            }
        }
    } else {
        shaderPipeline->setUniformValue(ubufData, inPropertyName, propertyValue, inPropertyType);
    }
}

void QSSGCustomMaterialSystem::applyRhiShaderPropertyValues(char *ubufData,
                                                            const QSSGRenderCustomMaterial &material,
                                                            const QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline)
{
    const auto &properties = material.m_properties;
    for (const auto &prop : properties)
        setShaderResources(ubufData, material, prop.name, prop.value, prop.shaderDataType, shaderPipeline);

    const auto textProps = material.m_textureProperties;
    for (const auto &prop : textProps)
        setShaderResources(ubufData, material, prop.name, QVariant::fromValue((void *)&prop), prop.shaderDataType, shaderPipeline);
}

void QSSGCustomMaterialSystem::rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                             QSSGSubsetRenderable &renderable,
                                             QSSGLayerRenderData &inData,
                                             bool *needsSetViewport,
                                             int cubeFace,
                                             QSSGRhiGraphicsPipelineState *state)
{
    QRhiGraphicsPipeline *ps = renderable.rhiRenderData.mainPass.pipeline;
    QRhiShaderResourceBindings *srb = renderable.rhiRenderData.mainPass.srb;

    if (cubeFace >= 0) {
        ps = renderable.rhiRenderData.reflectionPass.pipeline;
        srb = renderable.rhiRenderData.reflectionPass.srb[cubeFace];
    }

    if (!ps || !srb)
        return;

    QRhiBuffer *vertexBuffer = renderable.subset.rhi.vertexBuffer->buffer();
    QRhiBuffer *indexBuffer = renderable.subset.rhi.indexBuffer ? renderable.subset.rhi.indexBuffer->buffer() : nullptr;

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->setGraphicsPipeline(ps);
    cb->setShaderResources(srb);

    if (*needsSetViewport) {
        if (!state)
            cb->setViewport(rhiCtx->graphicsPipelineState(&inData)->viewport);
        else
            cb->setViewport(state->viewport);
        *needsSetViewport = false;
    }

    QRhiCommandBuffer::VertexInput vertexBuffers[2];
    int vertexBufferCount = 1;
    vertexBuffers[0] = QRhiCommandBuffer::VertexInput(vertexBuffer, 0);
    quint32 instances = 1;
    if (renderable.modelContext.model.instancing()) {
        instances = renderable.modelContext.model.instanceCount();
        vertexBuffers[1] = QRhiCommandBuffer::VertexInput(renderable.instanceBuffer, 0);
        vertexBufferCount = 2;
    }
    if (indexBuffer) {
        cb->setVertexInput(0, vertexBufferCount, vertexBuffers, indexBuffer, 0, renderable.subset.rhi.indexBuffer->indexFormat());
        cb->drawIndexed(renderable.subset.count, instances, renderable.subset.offset);
        QSSGRHICTX_STAT(rhiCtx, drawIndexed(renderable.subset.count, instances));
    } else {
        cb->setVertexInput(0, vertexBufferCount, vertexBuffers);
        cb->draw(renderable.subset.count, instances, renderable.subset.offset);
        QSSGRHICTX_STAT(rhiCtx, draw(renderable.subset.count, instances));
    }
}

QT_END_NAMESPACE
