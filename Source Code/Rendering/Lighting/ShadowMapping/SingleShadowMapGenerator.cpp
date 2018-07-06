#include "stdafx.h"

#include "Headers/SingleShadowMapGenerator.h"

#include "Core/Headers/StringHelper.h"
#include "Scenes/Headers/SceneState.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

SingleShadowMapGenerator::SingleShadowMapGenerator(GFXDevice& context)
    : ShadowMapGenerator(context)
{
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "Single Shadow Map");
}

void SingleShadowMapGenerator::render(const Camera& playerCamera, Light& light, U32 passIdx, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(playerCamera);

    ShadowCameraPool& shadowCameras = light.shadowCameras();
    shadowCameras[0]->lookAt(light.getPosition(), light.getSpotDirection());
    shadowCameras[0]->setProjection(1.0f, 90.0f, vec2<F32>(1.0, light.getRange()));

    RenderPassManager& passMgr = _context.parent().renderPassManager();
    RenderPassManager::PassParams params;
    params._doPrePass = false;
    params._occlusionCull = false;
    params._camera = shadowCameras[0];
    params._stage = RenderStage::SHADOW;
    params._target = RenderTargetID(RenderTargetUsage::SHADOW, to_base(ShadowType::SINGLE));
    params._drawPolicy = &RenderTarget::defaultPolicy();
    params._passIndex = passIdx;
    params._passVariant = to_U8(light.getLightType());

    passMgr.doCustomPass(params, bufferInOut);
}

};