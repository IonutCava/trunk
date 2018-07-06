#include "Headers/CubeShadowMap.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"

CubeShadowMap::CubeShadowMap(Light* light, Camera* shadowCamera) : ShadowMap(light, shadowCamera, SHADOW_TYPE_CubeMap)
{
    PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getGUID(), "Single Shadow Map");
	TextureDescriptor depthMapDescriptor(TEXTURE_CUBE_MAP, DEPTH_COMPONENT, UNSIGNED_INT); ///Default filters, LINEAR is OK for this

	SamplerDescriptor depthMapSampler;
	depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
	depthMapSampler.toggleMipMaps(false);
	depthMapSampler._useRefCompare = true; //< Use compare function
	depthMapSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
	depthMapDescriptor.setSampler(depthMapSampler);

	_depthMap = GFX_DEVICE.newFB();
	_depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth);
	_depthMap->toggleColorWrites(false);
}

CubeShadowMap::~CubeShadowMap()
{
}

void CubeShadowMap::init(ShadowMapInfo* const smi){
    resolution(smi->resolution(), _light->shadowMapResolutionFactor());
    _init = true;
}

void CubeShadowMap::resolution(U16 resolution, U8 resolutionFactor){
    U16 resolutionTemp = resolution * resolutionFactor;
    if (resolutionTemp != _resolution){
        _resolution = resolutionTemp;
		///Initialize the FB's with a variable resolution
        PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FB"), _light->getGUID());
        _depthMap->Create(_resolution, _resolution);
	}
    ShadowMap::resolution(resolution, resolutionFactor);
}

void CubeShadowMap::render(SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction){
    // Only if we have a valid callback;
	if(sceneRenderFunction.empty()) {
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getGUID());
		return;
	}

    GFX_DEVICE.generateCubeMap(*_depthMap, _light->getPosition(), sceneRenderFunction, SHADOW_STAGE);
}