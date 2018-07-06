#include "Headers/CubeShadowMap.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Headers/Frustum.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"

CubeShadowMap::CubeShadowMap(Light* light) : ShadowMap(light)
{
	_maxResolution = 0;
	_resolutionFactor = ParamHandler::getInstance().getParam<U8>("rendering.shadowResolutionFactor");
	CLAMP<F32>(_resolutionFactor,0.001f, 1.0f);
	PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FBO"), light->getId(), "Single Shadow Map");
	TextureDescriptor depthMapDescriptor(TEXTURE_2D, 
										 DEPTH_COMPONENT,
										 DEPTH_COMPONENT,
										 UNSIGNED_BYTE); ///Default filters, LINEAR is OK for this
	depthMapDescriptor.setWrapMode(TEXTURE_CLAMP_TO_EDGE,TEXTURE_CLAMP_TO_EDGE);
	depthMapDescriptor._useRefCompare = true; //< Use compare function
	depthMapDescriptor._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
	depthMapDescriptor._depthCompareMode = LUMINANCE;
	
	_depthMap = GFX_DEVICE.newFBO(FBO_CUBE_DEPTH);
	_depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth);
	_depthMap->toggleColorWrites(false);
}

CubeShadowMap::~CubeShadowMap()
{
}

void CubeShadowMap::resolution(U16 resolution,SceneRenderState* sceneRenderState){
	U8 resolutionFactorTemp = sceneRenderState->shadowMapResolutionFactor();
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
	ShadowMap::resolution(resolution,sceneRenderState);
}

void CubeShadowMap::render(SceneRenderState* renderState, boost::function0<void> sceneRenderFunction){
///Only if we have a valid callback;
	if(sceneRenderFunction.empty()) {
		ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
		return;
	}
	
	_callback = sceneRenderFunction;
	renderInternal(renderState);
}

void CubeShadowMap::renderInternal(SceneRenderState* renderState) const {
	
	///Only use cube map depth map
	if(_depthMap->getType() != FBO_CUBE_DEPTH){
		ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FBO_CUBEMAP_SHADOW"));
		return;
	}
	vec3<F32> lightPosition = _light->getPosition();
	Camera* activeCamera = GET_ACTIVE_SCENE()->renderState()->getCamera();
	///Get some global vars. We limit ourselves to drawing only the objects in the light's range. If range is infinit (-1) we use the GFX limit
	ParamHandler& par = ParamHandler::getInstance();
	F32 zNear  = Frustum::getInstance().getZPlanes().x;
	F32 zFar   = _light->getRange();
	if(zFar < zNear){
		zFar = Frustum::getInstance().getZPlanes().y;
	}
	///Save our current camera settings
	activeCamera->SaveCamera();
	///And save all camera transform matrices
	GFX_DEVICE.lockModelView();
	GFX_DEVICE.lockProjection();
	///set a 90 degree vertical FoV perspective projection
	GFX_DEVICE.setPerspectiveProjection(90.0,1,vec2<F32>(zNear,zFar));
	///Save our old rendering stage
	RenderStage prev = GFX_DEVICE.getRenderStage();
	///And set the current render stage to 
	GFX_DEVICE.setRenderStage(SHADOW_STAGE);
	///For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
	for(U8 i = 0; i < 6; i++){
		///Set the correct camera orientation and position for current face
		activeCamera->RenderLookAtToCubeMap( lightPosition, i );
		///Bind our FBO's current face
		_depthMap->Begin(i);
			///draw our scene
			
			///Don't need to override cubemap rendering callback
			if(_callback.empty()){
				Scene* activeScene = GET_ACTIVE_SCENE();
				GFX_DEVICE.render(SCENE_GRAPH_UPDATE(activeScene->getSceneGraph()), activeScene->renderState());
			}else{
				GFX_DEVICE.render(_callback, GET_ACTIVE_SCENE()->renderState());
			}
		///Unbind this face
		_depthMap->End(i);
	}
	///Return to our previous rendering stage
	GFX_DEVICE.setRenderStage(prev);
	///Restore transfom matrices
	GFX_DEVICE.releaseProjection();
	GFX_DEVICE.releaseModelView();
	///And restore camera
	activeCamera->RestoreCamera();
}