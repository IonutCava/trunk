#include "Headers/LightManager.h"
#include "Headers/SceneManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"

LightManager::LightManager() : FrameListener(),
							   _shadowMapsEnabled(true),
							   _previewShadowMaps(false),
							   _dominantLight(NULL)
{
}

LightManager::~LightManager()
{
	clear();
}

void LightManager::init(){
	REGISTER_FRAME_LISTENER(&(this->getInstance()));
}

bool LightManager::clear(){
	//Lights are removed by the sceneGraph
	for_each(LightMap::value_type& it, _lights){
		//in case we had some light hanging
		RemoveResource(it.second);
	}

	_lights.clear();

	return _lights.empty();
}

bool LightManager::addLight(Light* const light){
	if(light->getId() == 0)
		light->setId(generateNewID());
	
	light->addShadowMapInfo(New ShadowMapInfo(light));

	if(_lights.find(light->getId()) != _lights.end()){
		ERROR_FN(Locale::get("ERROR_LIGHT_MANAGER_DUPLICATE"),light->getId());
		return false;
	}

	_lights.insert(std::make_pair(light->getId(),light));
	return true;
}

bool LightManager::removeLight(U32 lightId){
	LightMap::const_iterator it = _lights.find(lightId);

	if(it == _lights.end()){
		ERROR_FN(Locale::get("ERROR_LIGHT_MANAGER_REMOVE_LIGHT"),lightId);
		return false;
	}

	(it->second)->removeShadowMapInfo();
	_lights.erase(lightId); //remove it from the map
	return true;
}

U32 LightManager::generateNewID(){
	U32 tempId = _lights.size();

	while(!checkId(tempId))
		tempId++;
	
	return tempId;
}

bool LightManager::checkId(U32 value){
	for_each(LightMap::value_type& it, _lights)
		if(it.second->getId() == value)
			return false;

	return true;
}

void LightManager::idle(){
	_shadowMapsEnabled = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows");
}

///Check light properties for every light (this is bound to the camera change listener group
///Update only if needed. Get projection and view matrices if they changed
///Also, search for the dominant light if any
void LightManager::update(){
	for_each(Light* light, _currLightsPerNode){
		light->updateState();
		if(!_dominantLight){ //if we do not have a dominant light registered, search for one
			if(light->getLightMode() == LIGHT_MODE_DOMINANT){
				//setting a light as dominant, will automatically inform the lightmanager, but just in case, make sure
				_dominantLight = light;
			}
		}
	}
}

///When pre-rendering is done, the Light Manager will generate the shadow maps
/// Returning false in any of the FrameListener methods will exit the entire application!
bool LightManager::framePreRenderEnded(const FrameEvent& evt){
	if(!_shadowMapsEnabled) 
		return true;

	//Stop if we have shadows disabled
	_lightProjectionMatricesCache.resize(Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
	//Tell the engine that we are drawing to depth maps
	RenderStage previousRS = GFX_DEVICE.getRenderStage();
	//set the current render stage to SHADOW_STAGE
	GFX_DEVICE.setRenderStage(SHADOW_STAGE);
	// if we have a dominant light, generate only it's shadows
	if(_dominantLight){
		 // When the entire scene is ready for rendering, generate the shadowmaps
		_dominantLight->generateShadowMaps(GET_ACTIVE_SCENE()->renderState());
	}else{
		//generate shadowmaps for each light
		for_each(LightMap::value_type& light, _lights){
			light.second->generateShadowMaps(GET_ACTIVE_SCENE()->renderState());
		}
	}
	//Revert back to the previous stage
	GFX_DEVICE.setRenderStage(previousRS);

	return true;
}

void LightManager::previewShadowMaps(Light* light) {
	//Stop if we have shadows disabled
	if(!_shadowMapsEnabled || !_previewShadowMaps ||  !GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE))
		return;

	Light* localLight = light;

	if(_dominantLight){
		localLight = _dominantLight;
	}else{
		if(localLight == NULL)	
			localLight = _lights[0];
	}

	if(!localLight->castsShadows())
		return;

	localLight->getShadowMapInfo()->getShadowMap()->previewShadowMaps();
}

//If we have computed shadowmaps, bind them before rendering any geometry;
//Always bind shadowmaps to slots Config::MAX_TEXTURE_STORAGE, Config::MAX_TEXTURE_STORAGE+1, Config::MAX_TEXTURE_STORAGE+2 ...
void LightManager::bindDepthMaps(Light* light, U8 lightIndex, U8 offset, bool overrideDominant){
	//Skip applying shadows if we are rendering to depth map, or we have shadows disabled
	if(!_shadowMapsEnabled || _lightProjectionMatricesCache.empty()) 
		return;

	Light* lightLocal = light;
	///If we have a dominant light, then both shadow casting lights are the same = the dominant one
	///Shadow map binding has a failsafe check for this, so it's ok to call bind twice
	if(_dominantLight && !overrideDominant)
		lightLocal = _dominantLight;
	
	if(!lightLocal->castsShadows())
		return;

	if(lightLocal->getLightType() == LIGHT_TYPE_DIRECTIONAL)
		offset = Config::MAX_TEXTURE_STORAGE + Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE;
	
	if(lightLocal->getLightType() == LIGHT_TYPE_POINT)
		offset = Config::MAX_TEXTURE_STORAGE + Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE + 1;
	
	_lightProjectionMatricesCache[lightIndex] = lightLocal->getLightProjectionMatrix();

	lightLocal->getShadowMapInfo()->getShadowMap()->Bind(offset);
}

void LightManager::unbindDepthMaps(Light* light, U8 offset, bool overrideDominant){
    if(!_shadowMapsEnabled || _lightProjectionMatricesCache.empty()) 
		return;

	Light* lightLocal = light;

	if(_dominantLight && !overrideDominant)
		lightLocal = _dominantLight;
	
	if(!lightLocal->castsShadows())
		return;

	if(lightLocal->getLightType() == LIGHT_TYPE_DIRECTIONAL)
		offset =  Config::MAX_TEXTURE_STORAGE + Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE;
	
	if(lightLocal->getLightType() == LIGHT_TYPE_POINT)
		offset = Config::MAX_TEXTURE_STORAGE + Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE + 1;
	
	lightLocal->getShadowMapInfo()->getShadowMap()->Unbind(offset);
}

bool LightManager::shadowMappingEnabled() const {
	if(!_shadowMapsEnabled)
		return false;

	for_each(LightMap::value_type light, _lights){
		if(!light.second->castsShadows()) 
			continue;

		ShadowMapInfo* smi = light.second->getShadowMapInfo();
		//no shadow info;
		if(!smi) 
			continue; 

		ShadowMap* sm = smi->getShadowMap();
		//no shadow map;
		if(!sm) 
			continue; 

		if(sm->getDepthMap()) 
			return true;
	}

	return false;
}

struct scoreCmpFnc{
	bool operator()(Light* const a, Light* const b) const {
		return (a)->getScore() < (b)->getScore();
	}
};

U8 LightManager::findLightsForSceneNode(SceneGraphNode* const node, LightType typeFilter){
	const vec3<F32> lumDot( 0.2125f, 0.7154f, 0.0721f );
	F32 luminace = 0.0f;
	F32 dist = 0.0f;
	F32 weight = 1.0f; // later
	U8 i = 0;
	vec3<F32> distToLight;
	// Reset light buffer
	_tempLightsPerNode.resize(_lights.size());
	_currLightsPerNode.resize(0);
	_currLightTypes.resize(0);
    _currLightsEnabled.resize(0);
	_currShadowLights.resize(0);
	// loop over every light in the scene
	// ToDo: add a grid based light search system? -Ionut
	for_each(LightMap::value_type& lightIt, _lights){
		Light* light = lightIt.second;
		if(!light->getEnabled())
			continue;
			
		LightType lType = light->getLightType();
		if(lType != LIGHT_TYPE_DIRECTIONAL )  {
			// get the luminosity.
			luminace = vec3<F32>(light->getVProperty(LIGHT_PROPERTY_DIFFUSE)).dot(lumDot);
			luminace *= light->getFProperty(LIGHT_PROPERTY_BRIGHTNESS);

	        F32 radiusSq = squared(light->getFProperty(LIGHT_PROPERTY_RANGE) + node->getBoundingSphere().getRadius());
			// get the distance to the light... score it 1 to 0 near to far.
			distToLight = node->getBoundingBox().getCenter() - light->getPosition();
            F32 distSq = radiusSq - distToLight.lengthSquared();

			if ( distSq > 0.0f )
			{
				dist = distSq /( 1000.0f * 1000.0f );
				CLAMP<F32>(dist, 0.0f, 1.0f );
			}
			light->setScore( luminace * weight * dist );

		}else{// directional
			light->setScore( weight );
		}
				
		// use type filter
		if((typeFilter != LIGHT_TYPE_PLACEHOLDER && lType == typeFilter) || //< wether we have the correct light type
			typeFilter == LIGHT_TYPE_PLACEHOLDER){ //< or we did not specify a type filter
			_tempLightsPerNode[i++] = light;
		}
	}

	// sort the lights by score
	std::sort(_tempLightsPerNode.begin(), 
		      _tempLightsPerNode.end(),
			  scoreCmpFnc());

	// create the light buffer for the specified node
	I32 maxLights = _tempLightsPerNode.size();
	CLAMP<I32>(maxLights, 0, Config::MAX_LIGHTS_PER_SCENE_NODE);
	for(U8 i = 0; i < maxLights; i++){
		_currLightsPerNode.push_back(_tempLightsPerNode[i]);
		_currLightTypes.push_back(_tempLightsPerNode[i]->getLightType());
        _currLightsEnabled.push_back(_tempLightsPerNode[i]->getEnabled() ? 1 : 0);
		_currShadowLights.push_back(_tempLightsPerNode[i]->castsShadows() ? 1 : 0);
	}

	return maxLights;
}