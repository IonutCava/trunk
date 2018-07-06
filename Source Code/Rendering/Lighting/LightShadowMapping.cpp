#include "Headers/Light.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/GFXDevice.h" 

///Shadow mapping callback is the function that draws the scene when we generate our shadow map
///Everything drawn in the callback will be rendered from the light's view and written to the depth buffer
///When we set the callback, it's obvious we need shadow maps for this light, so create the FBO's
void Light::setShadowMappingCallback(boost::function0<void> callback) {
	///Only if we have a valid callback;
	if(callback.empty()) {
		ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _id);
		return;
	}
	///The higher the shadow detail level, the bigger depth buffers we have
	F32 resolutionFactor = ParamHandler::getInstance().getParam<U8>("shadowDetailLevel");
	///If this is the first initialization pass, first = true;
	///If we changed the callback after initialization, first = false;
	bool firstPass = false;

	///If this is the first call, create depth maps;
	if(_depthMaps.empty()){
		///Create the FBO's
		PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FBO"), _id);
		for(U8 i = 0; i < 3; i++){
			_depthMaps.push_back(GFX_DEVICE.newFBO());
		}
		firstPass = true;
	}
	if(_resolutionFactor != resolutionFactor || firstPass){
		///Initialize the FBO's with a variable resolution
		PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FBO"), _id);
		_depthMaps[0]->Create(FBO_2D_DEPTH,2048/resolutionFactor,2048/resolutionFactor);
		_depthMaps[1]->Create(FBO_2D_DEPTH,1024/resolutionFactor,1024/resolutionFactor);
		_depthMaps[2]->Create(FBO_2D_DEPTH,512/resolutionFactor,512/resolutionFactor);
		_resolutionFactor = resolutionFactor;
	}

	///Finally assign the callback
	_callback = callback;
}


///This function moves the camera to the light's view and set's it's frustum according to the current pass
///The more depthmpas, the more transforms
void Light::generateShadowMaps(const vec3<F32>& camEyePos){
	GFXDevice& gfx = GFX_DEVICE;
	if(!_castsShadows) return;
	///Stop if the callback is invalid
	if(_callback.empty()) return;
	///Get our eye view
	_eyePos = camEyePos;
	///For every depth map
	///Lock our projection matrix so no changes will be permanent during the rest of the frame
	gfx.lockProjection();
	///Lock our model view matrix so no camera transforms will be saved beyond this light's scope
	gfx.lockModelView();
	_zNear = _par.getParam<F32>("zNear");
	_zFar  = _par.getParam<F32>("zFar");
	///Set the camera to the light's view
	setCameraToLightView();
	///For each depth pass
	for(U8 i = 0; i < _depthMaps.size(); i++){
		///Set the appropriate projection
		renderFromLightView(i);
		///bind the associated depth map
		_depthMaps[i]->Begin();
			///draw the scene
			_callback();
		///unbind the associated depth map
		_depthMaps[i]->End();
	}
	///get all the required information (light MVP matrix for example) 
	///and restore to the proper camera view
	setCameraToSceneView();
	///Undo all modifications to the Projection Matrix
	gfx.releaseProjection();
	///Undo all modifications to the ModelView Matrix
	gfx.releaseModelView();
	///Restore our view frustum
	Frustum::getInstance().Extract(_eyePos);
}

///Step I : Set our camera to appropriate position
void Light::setCameraToLightView(){
	///Depending on our light type, perform the necessary camera transformations
	switch(_type){
		case LIGHT_OMNI: ///ToDo: This needs CubeMap depthBuffers!! -Ionut
			setCameraToLightViewOmni();
		break;
		case LIGHT_SPOT:
			setCameraToLightViewSpot();
		break;
		default:
		case LIGHT_DIRECTIONAL:
			setCameraToLightViewDirectional();
		break;
	};
}

void Light::setCameraToLightViewOmni(){
}

void Light::setCameraToLightViewSpot(){
	///Trim the w parameter from the position
	vec3<F32> lightPosition = vec3<F32>(_lightProperties_v[LIGHT_POSITION]);
	vec3<F32> spotTarget = vec3<F32>(_lightProperties_v[LIGHT_SPOT_DIRECTION]);
	///A spot light has a target and a position
	_lightPos = vec3<F32>(lightPosition);
	///Tell our rendering API to move the camera
	GFX_DEVICE.lookAt(_lightPos,    //the light's  position
					  spotTarget);  //the light's target
}

float split_weight = 0.75f;
I8 cur_num_splits = 3;
/// updateSplitDist computes the near and far distances for every frustum slice
/// in camera eye space - that is, at what distance does a slice start and end
/// Compute the 8 corner points of the current view frustum
void updateFrustumPoints(frustum &f, vec3<F32> &center, vec3<F32> &view_dir){
	vec3<F32> up(0.0f, 1.0f, 0.0f);
	vec3<F32> view_dir_temp = view_dir;
	view_dir_temp.cross(up);
	vec3<F32> right = view_dir_temp;
	right.normalize();
	vec3<F32> right_temp = right;
	vec3<F32> fc = center + view_dir*f.fard;
	vec3<F32> nc = center + view_dir*f.neard;

	
	right_temp.cross(view_dir);
	up = right_temp;
	up.normalize();

	/// these heights and widths are half the heights and widths of
	/// the near and far plane rectangles
	F32 near_height = tan(f.fov/2.0f) * f.neard;
	F32 near_width = near_height * f.ratio;
	F32 far_height = tan(f.fov/2.0f) * f.fard;
	F32 far_width = far_height * f.ratio;

	f.point[0] = nc - up*near_height - right*near_width;
	f.point[1] = nc + up*near_height - right*near_width;
	f.point[2] = nc + up*near_height + right*near_width;
	f.point[3] = nc - up*near_height + right*near_width;

	f.point[4] = fc - up*far_height - right*far_width;
	f.point[5] = fc + up*far_height - right*far_width;
	f.point[6] = fc + up*far_height + right*far_width;
	f.point[7] = fc - up*far_height + right*far_width;
}

void updateSplitDist(frustum f[3], F32 nd, F32 fd){
	F32 lambda = split_weight;
	F32 ratio = fd/nd;
	f[0].neard = nd;

	for(U8 i=1; i<cur_num_splits; i++){
		F32 si = i / (F32)cur_num_splits;

		f[i].neard = lambda*(nd*powf(ratio, si)) + (1-lambda)*(nd + (fd - nd)*si);
		f[i-1].fard = f[i].neard * 1.005f;
	}
	f[cur_num_splits-1].fard = fd;
}

void Light::setCameraToLightViewDirectional(){
	///Trim the w parameter from the position
	vec3<F32> lightPosition = vec3<F32>(_lightProperties_v[LIGHT_POSITION]);
	///Set the virtual light position 500 units above our eye position
	///This is one example why we have different setup functions for each light type
	///This isn't valid for omni or spot
	_lightPos = vec3<F32>(_eyePos.x - 500*lightPosition.x,	
					      _eyePos.y - 500*lightPosition.y,
					      _eyePos.z - 500*lightPosition.z);
	  
	///Tell our rendering API to move the camera
	GFX_DEVICE.lookAt(_lightPos,    //the light's virtual position
					  _eyePos);     //the light's target
}

///We need to render the scene from the light's perspective
///This varies depending on the current pass:
///--The higher the pass, the closer the view
void Light::renderFromLightView(U8 depthPass){
	
	//Set the camera to the light's view depending on it's type.
	//This switch format is appropriate here because it's easily expandable
	switch(_type){
		case LIGHT_OMNI:
			//ToDo: This needs CubeMap depthBuffers!! -Ionut
			renderFromLightViewOmni(depthPass);
		break;
		case LIGHT_SPOT:
			renderFromLightViewSpot(depthPass);
		break;
		default:
		case LIGHT_DIRECTIONAL:
			renderFromLightViewDirectional(depthPass);
		break;
	};

}

void Light::setCameraToSceneView(){
	GFXDevice& gfx = GFX_DEVICE;
	Frustum& frust = Frustum::getInstance();
	///Set the ortho projection so that it encompasses the entire scene
	gfx.setOrthoProjection(vec4<F32>(-1.0, 1.0, -1.0, 1.0), _zPlanes);
	///Extract the view frustum from this projection mode
	frust.Extract(_eyePos - _lightPos);
	///get the MVP from the new Frustum as this is the light's full MVP
	///save it as the current light MVP
	gfx.setLightProjectionMatrix(frust.getModelViewProjectionMatrix());
}


void Light::renderFromLightViewOmni(U8 depthPass){
	GFX_DEVICE.lookAt(_lightPos,vec3<F32>(0,0,0));
}

void Light::renderFromLightViewSpot(U8 depthPass){
	GFX_DEVICE.lookAt(_lightPos,vec3<F32>(0,0,0)/*target*/);
}

void Light::renderFromLightViewDirectional(U8 depthPass){
	///Some ortho values to create closer and closer light views
	D32 lightOrtho[3] = {5.0, 10.0, 50.0};
	///ToDo: Near and far planes. Should optimize later! -Ionut
	_zPlanes = vec2<F32>(_zNear, _zFar);
	///Set the current projection depending on the current depth pass
	GFX_DEVICE.setOrthoProjection(vec4<F32>(-lightOrtho[depthPass], 
											 lightOrtho[depthPass],
											-lightOrtho[depthPass], 
											 lightOrtho[depthPass]),
											_zPlanes);
	Frustum::getInstance().Extract(_eyePos - _lightPos);
}