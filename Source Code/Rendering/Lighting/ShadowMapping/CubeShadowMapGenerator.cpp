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
    : ShadowMapGenerator(context, ShadowType::CUBEMAP)
{
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "Single Shadow Map");
}

void CubeShadowMapGenerator::render(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(playerCamera);

    const vec3<F32> lightPos = light.getSGN().get<TransformComponent>()->getPosition();

    auto& shadowCameras = ShadowMap::shadowCameras(ShadowType::CUBEMAP);

    std::array<Camera*, 6> cameras;
#if 1
    for (U8 i = 0; i < 6; ++i) {
        cameras[i] = shadowCameras[i];
    }
#else
    std::copy_n(std::begin(shadowCameras), 6, std::begin(cameras));
#endif
    _context.generateCubeMap(RenderTargetID(RenderTargetUsage::SHADOW, to_base(ShadowType::CUBEMAP)),
                             light.getShadowOffset(),
                             light.getSGN().get<TransformComponent>()->getPosition(),
                             vec2<F32>(0.01f, light.range() * 1.25f),
                             {RenderStage::SHADOW, RenderPassType::COUNT, to_U8(light.getLightType()), lightIndex},
                             bufferInOut,
                             cameras,
                             &light.getSGN());

    for (U8 i = 0; i < 6; ++i) {
        light.setShadowLightPos(i, lightPos);
        const mat4<F32> matVP = mat4<F32>::Multiply(shadowCameras[i]->getViewMatrix(), shadowCameras[i]->getProjectionMatrix());
        light.setShadowVPMatrix(i, mat4<F32>::Multiply(matVP, MAT4_BIAS));
    }
}

};