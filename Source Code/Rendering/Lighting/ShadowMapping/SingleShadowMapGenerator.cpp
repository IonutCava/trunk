#include "stdafx.h"

#include "Headers/SingleShadowMapGenerator.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Scenes/Headers/SceneState.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

SingleShadowMapGenerator::SingleShadowMapGenerator(GFXDevice& context)
    : ShadowMapGenerator(context, ShadowType::SINGLE)
{
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "Single Shadow Map");
}

void SingleShadowMapGenerator::render(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(playerCamera);

    const vec3<F32> lightPos = light.getSGN().get<TransformComponent>()->getPosition();

    auto& shadowCameras = ShadowMap::shadowCameras(ShadowType::SINGLE);
    mat4<F32> viewMatrix = shadowCameras[0]->lookAt(lightPos, light.getSGN().get<TransformComponent>()->getOrientation() * WORLD_Z_NEG_AXIS);
    mat4<F32> projectionMatrix = shadowCameras[0]->setProjection(1.0f, 90.0f, vec2<F32>(0.01f, light.range() * 1.25f));
    
    light.setShadowLightPos(0, lightPos);
    light.setShadowVPMatrix(0, mat4<F32>::Multiply(mat4<F32>::Multiply(viewMatrix, projectionMatrix), MAT4_BIAS));

    RenderPassManager* passMgr = _context.parent().renderPassManager();

    RenderPassManager::PassParams params = {};
    params._sourceNode = &light.getSGN();
    params._camera = shadowCameras[0];
    params._stagePass = { RenderStage::SHADOW, RenderPassType::COUNT, to_U8(light.getLightType()), lightIndex };
    params._target = RenderTargetID(RenderTargetUsage::SHADOW, to_base(_type));
    params._passName = "SingleShadowMap";

    passMgr->doCustomPass(params, bufferInOut);
}

};