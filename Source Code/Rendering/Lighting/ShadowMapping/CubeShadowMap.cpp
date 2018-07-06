#include "Headers/CubeShadowMap.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Headers/Frustum.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"

CubeShadowMap::CubeShadowMap(Light* light) : ShadowMap(light, SHADOW_TYPE_CubeMap)
{
	_maxResolution = 0;
	_resolutionFactor = ParamHandler::getInstance().getParam<U8>("rendering.shadowResolutionFactor");
	CLAMP<F32>(_resolutionFactor,0.001f, 1.0f);
	PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getId(), "Single Shadow Map");
	TextureDescriptor depthMapDescriptor(TEXTURE_CUBE_MAP,
										 DEPTH_COMPONENT,
										 DEPTH_COMPONENT,
										 UNSIGNED_INT); ///Default filters, LINEAR is OK for this

	SamplerDescriptor depthMapSampler;
	depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
	depthMapSampler.toggleMipMaps(false);
	depthMapSampler._useRefCompare = true; //< Use compare function
	depthMapSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
	depthMapDescriptor.setSampler(depthMapSampler);

	_depthMap = GFX_DEVICE.newFB(FB_CUBE_DEPTH);
	_depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth);
	_depthMap->toggleColorWrites(false);
}

CubeShadowMap::~CubeShadowMap()
{
}

void CubeShadowMap::init(ShadowMapInfo* const smi){
    resolution(smi->resolution(), smi->resolutionFactor());
    _init = true;
}

void CubeShadowMap::resolution(U16 resolution, F32 resolutionFactor){
    U8 resolutionFactorTemp = resolutionFactor;
	CLAMP<U8>(resolutionFactorTemp, 1, 4);
	U16 maxResolutionTemp = resolution;
	if(resolutionFactorTemp != _resolutionFactor || _maxResolution != maxResolutionTemp){
		_resolutionFactor = resolutionFactorTemp;
		_maxResolution = maxResolutionTemp;
		///Initialize the FB's with a variable resolution
		PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FB"), _light->getId());
		U16 shadowMapDimension = _maxResolution/_resolutionFactor;
		_depthMap->Create(shadowMapDimension,shadowMapDimension);
	}
    ShadowMap::resolution(resolution, resolutionFactor);
}

void CubeShadowMap::render(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction){
    // Only if we have a valid callback;
	if(sceneRenderFunction.empty()) {
		ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
		return;
	}

    GFX_DEVICE.generateCubeMap(*_depthMap, _light->getPosition(), sceneRenderFunction, SHADOW_STAGE);
}