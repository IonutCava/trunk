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
    Console::printfn(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getGUID(),
                     "Single Shadow Map");
    // Default filters, LINEAR is OK for this
    TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_CUBE_MAP,
                                         GFXImageFormat::DEPTH_COMPONENT,
                                         GFXDataFormat::UNSIGNED_INT);

    SamplerDescriptor depthMapSampler;
    depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(false);
    depthMapSampler._useRefCompare = true;  //< Use compare function
    depthMapSampler._cmpFunc =
        ComparisonFunction::LEQUAL;  //< Use less or equal
    depthMapDescriptor.setSampler(depthMapSampler);

    _depthMap = GFX_DEVICE.newFB();
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::AttachmentType::Depth);
    _depthMap->toggleColorWrites(false);
}

CubeShadowMap::~CubeShadowMap() {}

void CubeShadowMap::init(ShadowMapInfo* const smi) {
    resolution(smi->resolution(), _light->shadowMapResolutionFactor());
    _init = true;
}

void CubeShadowMap::resolution(U16 resolution, U8 resolutionFactor) {
    U16 resolutionTemp = resolution * resolutionFactor;
    if (resolutionTemp != _resolution) {
        _resolution = resolutionTemp;
        // Initialize the FB's with a variable resolution
        Console::printfn(Locale::get("LIGHT_INIT_SHADOW_FB"),
                         _light->getGUID());
        _depthMap->Create(_resolution, _resolution);
    }
    ShadowMap::resolution(resolution, resolutionFactor);
}

void CubeShadowMap::render(SceneRenderState& renderState,
                           const DELEGATE_CBK<>& sceneRenderFunction) {
    // Only if we have a valid callback;
    if (!sceneRenderFunction) {
        Console::errorfn(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"),
                         _light->getGUID());
        return;
    }

    GFX_DEVICE.generateCubeMap(*_depthMap,
                                _light->getPosition(),
                                sceneRenderFunction,
                                vec2<F32>(0.1f, _light->getRange()),
                                RenderStage::SHADOW);
}
};