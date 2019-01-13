#include "stdafx.h"

#include "Headers/CubeShadowMapGenerator.h"

#include "Core/Headers/StringHelper.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

CubeShadowMapGenerator::CubeShadowMapGenerator(GFXDevice& context)
    : ShadowMapGenerator(context)
{
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "Single Shadow Map");
}

void CubeShadowMapGenerator::render(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(playerCamera);

    _context.generateCubeMap(RenderTargetID(RenderTargetUsage::SHADOW, to_base(ShadowType::CUBEMAP)),
                             light.getShadowOffset(),
                             light.getSGN().get<TransformComponent>()->getPosition(),
                             vec2<F32>(0.1f, light.getRange()),
                             RenderStagePass(RenderStage::SHADOW, RenderPassType::PRE_PASS, to_U8(light.getLightType())),
                             (lightIndex * Config::Lighting::MAX_SHADOW_CASTING_LIGHTS),
                             bufferInOut,
                             &light.getSGN(),
                             light.shadowCameras()[0]);
}

};