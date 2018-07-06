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
	PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FBO"), light->getId(), "Single Shadow Map");
	TextureDescriptor depthMapDescriptor(TEXTURE_2D,
										 DEPTH_COMPONENT,
										 DEPTH_COMPONENT24,
										 UNSIGNED_BYTE); ///Default filters, LINEAR is OK for this

	SamplerDescriptor depthMapSampler;
	depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
	depthMapSampler.toggleMipMaps(false);
	depthMapSampler._useRefCompare = true; //< Use compare function
	depthMapSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
	depthMapSampler._depthCompareMode = LUMINANCE;
	depthMapDescriptor.setSampler(depthMapSampler);

	_depthMap = GFX_DEVICE.newFBO(FBO_CUBE_DEPTH);
	_depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth);
	_depthMap->toggleColorWrites(false);
}

CubeShadowMap::~CubeShadowMap()
{
}

void CubeShadowMap::resolution(U16 resolution, const SceneRenderState& renderState){
	U8 resolutionFactorTemp = renderState.shadowMapResolutionFactor();
	CLAMP<U8>(resolutionFactorTemp, 1, 4);
	U16 maxResolutionTemp = resolution;
	if(resolutionFactorTemp != _resolutionFactor || _maxResolution != maxResolutionTemp){
		_resolutionFactor = resolutionFactorTemp;
		_maxResolution = maxResolutionTemp;
		///Initialize the FBO's with a variable resolution
		PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FBO"), _light->getId());
		U16 shadowMapDimension = _maxResolution/_resolutionFactor;
		_depthMap->Create(shadowMapDimension,shadowMapDimension,RGBA16F);
	}
	ShadowMap::resolution(resolution, renderState);
}

void CubeShadowMap::render(const SceneRenderState& renderState, boost::function0<void> sceneRenderFunction){
///Only if we have a valid callback;
	if(sceneRenderFunction.empty()) {
		ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
		return;
	}

	_callback = sceneRenderFunction;
	renderInternal(renderState);
}

void CubeShadowMap::renderInternal(const SceneRenderState& renderState) const {
	//Get some global vars. We limit ourselves to drawing only the objects in the light's range. If range is infinit (-1) we use the GFX limit
	F32 zNear  = Frustum::getInstance().getZPlanes().x;
	F32 zFar   = _light->getRange();
	F32 oldzFar = Frustum::getInstance().getZPlanes().y;
	if(zFar < zNear){
		zFar = oldzFar;
	}
	Frustum::getInstance().setZPlanes(vec2<F32>(zNear,zFar));

	GFX_DEVICE.generateCubeMap(*_depthMap, _light->getPosition(), _callback, SHADOW_STAGE);

	Frustum::getInstance().setZPlanes(vec2<F32>(zNear,oldzFar));
}