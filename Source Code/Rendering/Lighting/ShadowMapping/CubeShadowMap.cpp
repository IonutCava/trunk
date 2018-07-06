#include "Headers/CubeShadowMap.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

CubeShadowMap::CubeShadowMap(Light* light, Camera* shadowCamera)
    : ShadowMap(light, shadowCamera, ShadowType::CUBEMAP) {
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(), "Single Shadow Map");

}

CubeShadowMap::~CubeShadowMap() {}

void CubeShadowMap::init(ShadowMapInfo* const smi) {
    _init = true;
}

void CubeShadowMap::render(SceneRenderState& renderState) {
    GFX_DEVICE.generateCubeMap(*getDepthMap(),
                                _arrayOffset,
                                _light->getPosition(),
                                vec2<F32>(0.1f, _light->getRange()),
                                RenderStage::SHADOW);
}
};