#include "stdafx.h"

#include "Headers/CubeShadowMapGenerator.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {
namespace {
    const RenderTargetID g_depthMapID(RenderTargetUsage::SHADOW, to_base(ShadowType::CUBEMAP));
    Configuration::Rendering::ShadowMapping g_shadowSettings;
};

CubeShadowMapGenerator::CubeShadowMapGenerator(GFXDevice& context)
    : ShadowMapGenerator(context, ShadowType::CUBEMAP)
{
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "Single Shadow Map");
    g_shadowSettings = context.context().config().rendering.shadowMapping;
}

void CubeShadowMapGenerator::render(const Camera& playerCamera, Light& light, U16 lightIndex, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(playerCamera);

    const vec3<F32> lightPos = light.getSGN()->get<TransformComponent>()->getPosition();

    auto& shadowCameras = ShadowMap::shadowCameras(ShadowType::CUBEMAP);

    std::array<Camera*, 6> cameras = {};
    std::copy_n(std::begin(shadowCameras), std::min(cameras.size(),shadowCameras.size()), std::begin(cameras));

    GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand({ Util::StringFormat("Cube Shadow Pass Light: [ %d ]", lightIndex).c_str() }));

    _context.generateCubeMap(RenderTargetID(RenderTargetUsage::SHADOW, to_base(ShadowType::CUBEMAP)),
                             light.getShadowOffset(),
                             light.getSGN()->get<TransformComponent>()->getPosition(),
                             vec2<F32>(0.001f, light.range() * 1.1f),
                             RenderStagePass(RenderStage::SHADOW, RenderPassType::MAIN_PASS, to_U8(light.getLightType()), lightIndex),
                             bufferInOut,
                             cameras,
                             true,
                             light.getSGN());

    for (U8 i = 0; i < 6; ++i) {
        light.setShadowLightPos(i, lightPos);
        light.setShadowFloatValue(i, shadowCameras[i]->getZPlanes().y);
        const mat4<F32> matVP = mat4<F32>::Multiply(shadowCameras[i]->getViewMatrix(), shadowCameras[i]->getProjectionMatrix());
        light.setShadowVPMatrix(i, mat4<F32>::Multiply(matVP, MAT4_BIAS));
    }

    if (g_shadowSettings.point.enableBlurring) {
        NOP();
    }

    const RenderTarget& shadowMapRT = _context.renderTargetPool().renderTarget(g_depthMapID);
    const U16 layerOffset = light.getShadowOffset();
    const I32 layerCount = 1;

    GFX::ComputeMipMapsCommand computeMipMapsCommand = {};
    computeMipMapsCommand._texture = shadowMapRT.getAttachment(RTAttachmentType::Colour, 0).texture().get();
    computeMipMapsCommand._layerRange = vec2<U32>(light.getShadowOffset(), layerCount);
    computeMipMapsCommand._defer = false;
    GFX::EnqueueCommand(bufferInOut, computeMipMapsCommand);

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}

void CubeShadowMapGenerator::updateMSAASampleCount(U8 sampleCount) {
    if (_context.context().config().rendering.shadowMapping.point.MSAASamples != sampleCount) {
        _context.context().config().rendering.shadowMapping.point.MSAASamples = sampleCount;
    }
}
};