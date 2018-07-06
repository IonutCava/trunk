#include "Headers/Light.h"
#include "Hardware/Video/GFXDevice.h" 
#include "Managers/Headers/CameraManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"

//Shadow mapping callback is the function that draws the scene when we generate our shadow map
//Everything drawn in the callback will be rendered from the light's view and written to the depth buffer
//When we set the callback, it's obvious we need shadow maps for this light, so create the FBO's
void Light::setShadowMappingCallback(boost::function0<void> callback) {
	//Only if we have a valid callback;
	if(callback.empty()) {
		Console::getInstance().errorfn("Setting invalid shadow casting callback for light [ %d ]", _id);
		return;
	}
	F32 resolutionFactor = 1;
	//If this is the first initialization pass, first = true;
	//If we changed the callback after initialization, first = false;
	bool firstPass = false;
	//The higher the shadow detail level, the bigger depth buffers we have
	switch(ParamHandler::getInstance().getParam<U8>("shadowDetailLevel")){
		case LOW: {
			 //1/4 the shadowmap resolution
			resolutionFactor = 4;
		}break;
		case MEDIUM: {
			//1/2 the shadowmap resolution
			resolutionFactor = 2;
		}break;
		default:
		case HIGH: {
			//Full shadowmap resolution
			resolutionFactor = 1;
		}break;
	};
	//If this is the first call, create depth maps;
	if(_depthMaps.empty()){
		//Create the FBO's
		Console::getInstance().printfn("Creating the Frame Buffer Objects for Shadow Mapping for Light [ %d ]", _id);
		for(U8 i = 0; i < 3; i++){
			_depthMaps.push_back(GFXDevice::getInstance().newFBO());
		}
		firstPass = true;
	}
	if(_resolutionFactor != resolutionFactor || firstPass){
		//Initialize the FBO's with a variable resolution
		Console::getInstance().printfn("Initializing the Frame Buffer Objects for Shadow Mapping for Light [ %d ]", _id);
		_depthMaps[0]->Create(FrameBufferObject::FBO_2D_DEPTH,2048/resolutionFactor,2048/resolutionFactor);
		_depthMaps[1]->Create(FrameBufferObject::FBO_2D_DEPTH,1024/resolutionFactor,1024/resolutionFactor);
		_depthMaps[2]->Create(FrameBufferObject::FBO_2D_DEPTH,512/resolutionFactor,512/resolutionFactor);
		_resolutionFactor = resolutionFactor;
	}

	//Finally assign the callback
	_callback = callback;
}


//This function moves the camera to the light's view and set's it's frustum according to the current pass
//The more depthmpas, the more transforms
void Light::generateShadowMaps(){
	GFXDevice& gfx = GFXDevice::getInstance();
	if(!_castsShadows) return;
	//Stop if the callback is invalid
	if(_callback.empty()) return;
	//Get our eye view
	_eyePos = CameraManager::getInstance().getActiveCamera()->getEye();
	//For every depth map
	//Lock our projection matrix so no changes will be permanent during the rest of the frame
	gfx.lockProjection();
	//Lock our model view matrix so no camera transforms will be saved beyond this light's scope
	gfx.lockModelView();

	//Set the camera to the light's view
	setCameraToLightView();
	//For each depth pass
	for(U8 i = 0; i < _depthMaps.size(); i++){
		//Set the appropriate projection
		renderFromLightView(i);
		//bind the associated depth map
		_depthMaps[i]->Begin();
			//Clear the depth buffer
			//gfx.clearBuffers(GFXDevice::DEPTH_BUFFER);
			//draw the scene
			_callback();
		//unbind the associated depth map
		_depthMaps[i]->End();
	}
	//get all the required information (light MVP matrix for example) 
	//and restore to the proper camera view
	setCameraToSceneView();
	//Undo all modifications to the Projection Matrix
	gfx.releaseProjection();
	//Undo all modifications to the ModelView Matrix
	gfx.releaseModelView();
	//Restore our view frustum
	Frustum::getInstance().Extract(_eyePos);
}

//Step I : Set our camera to appropriate position
void Light::setCameraToLightView(){
	//Depending on our light type, perform the necessary camera transformations
	switch(_type){
		case LIGHT_OMNI: //ToDo: This needs CubeMap depthBuffers!! -Ionut
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
	//Trim the w parameter from the position
	vec3 lightPosition = vec3(_lightProperties_v["position"]);
	vec3 spotTarget = vec3(_lightProperties_v["target"]);
	//A spot light has a target and a position
	_lightPos = vec3(lightPosition);
	//Tell our rendering API to move the camera
	GFXDevice::getInstance().lookAt(_lightPos,    //the light's  position
									spotTarget);  //the light's target
}

void Light::setCameraToLightViewDirectional(){
	//Trim the w parameter from the position
	vec3 lightPosition = vec3(_lightProperties_v["position"]);
	//Set the virtual light position 500 units above our eye position
	//This is one example why we have different setup functions for each light type
	//This isn't valid for omni or spot
	_lightPos = vec3(_eyePos.x - 500*lightPosition.x,	
					 _eyePos.y - 500*lightPosition.y,
					 _eyePos.z - 500*lightPosition.z);
	
	//Tell our rendering API to move the camera
	GFXDevice::getInstance().lookAt(_lightPos,    //the light's virtual position
									_eyePos);     //the light's target
}

//We need to render the scene from the light's perspective
//This varies depending on the current pass:
//--The higher the pass, the closer the view
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
	GFXDevice& gfx = GFXDevice::getInstance();
	Frustum& frust = Frustum::getInstance();
	//Set the ortho projection so that it encompasses the entire scene
	gfx.setOrthoProjection(vec4(-1.0, 1.0, -1.0, 1.0), _zPlanes);
	//Extract the view frustum from this projection mode
	frust.Extract(_eyePos - _lightPos);
	//get the MVP from the new Frustum as this is the light's full MVP
	//save it as the current light MVP
	gfx.setLightProjectionMatrix(frust.getModelViewProjectionMatrix());
}


void Light::renderFromLightViewOmni(U8 depthPass){
	GFXDevice::getInstance().lookAt(_lightPos,vec3(0,0,0));
}

void Light::renderFromLightViewSpot(U8 depthPass){
	GFXDevice::getInstance().lookAt(_lightPos,vec3(0,0,0)/*target*/);
}

void Light::renderFromLightViewDirectional(U8 depthPass){
	ParamHandler& par = ParamHandler::getInstance();
	//Some ortho values to create closer and closer light views
	D32 lightOrtho[3] = {2.0, 10.0, 50.0};
	//ToDo: Near and far planes. Should optimize later! -Ionut
	_zPlanes = vec2(par.getParam<F32>("zNear"),par.getParam<F32>("zFar"));
	//Set the current projection depending on the current depth pass
	GFXDevice::getInstance().setOrthoProjection(vec4(-lightOrtho[depthPass], 
													  lightOrtho[depthPass],
													 -lightOrtho[depthPass], 
													  lightOrtho[depthPass]),
												_zPlanes);
	Frustum::getInstance().Extract(_eyePos - _lightPos);
}