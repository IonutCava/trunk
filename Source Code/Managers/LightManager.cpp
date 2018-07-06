#include "Headers/LightManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"

LightManager::LightManager() : _previewShadowMaps(false),
							   _dominantLight(NULL),
							   _shadowMapsEnabled(true),
							   _shadowArrayOffset(-1),
							   _shadowCubeOffset(-1)
{
}

LightManager::~LightManager(){
	clear();
}

void LightManager::setAmbientLight(const vec4<F32>& light){
	GFX_DEVICE.setAmbientLight(light);
}

void LightManager::init(){
	I32 baseOffset = ParamHandler::getInstance().getParam<I32>("GFX_DEVICE.maxTextureCombinedUnits",16);
	_shadowArrayOffset = baseOffset - 2;
	_shadowCubeOffset = baseOffset - 1;
}

bool LightManager::clear(){
	//Lights are removed by the sceneGraph
	for_each(LightMap::value_type& it, _lights){
		//in case we had some light hanging
		RemoveResource(it.second);
	}
	_lights.clear();
	if(_lights.empty()) return true;
	else return false;
}

bool LightManager::addLight(Light* const light){
	if(light->getId() == 0){
		light->setId(generateNewID());
	}
	light->addShadowMapInfo(New ShadowMapInfo(light));
	LightMap::iterator& it = _lights.find(light->getId());
	if(it != _lights.end()){
		ERROR_FN(Locale::get("ERROR_LIGHT_MANAGER_DUPLICATE"),light->getId());
		return false;
	}

	_lights.insert(std::make_pair(light->getId(),light));
	return true;
}

bool LightManager::removeLight(U32 lightId){
	LightMap::iterator it = _lights.find(lightId);

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
	while(!checkId(tempId)){
		tempId++;
	}
	return tempId;
}

bool LightManager::checkId(U32 value){
	for_each(LightMap::value_type& it, _lights){
		if(it.second->getId() == value){
			return false;
		}
	}
	return true;
}

void LightManager::idle(){
	_shadowMapsEnabled = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows");
}

///Check light properties for every light
///Update only if needed
///Also, search for the dominant light if any
void LightManager::update(bool force){
	if(!force){
		for_each(Light* light, _currLightsPerNode){
			light->updateState(force);
		}
	}else{
		for_each(LightMap::value_type& light, _lights){
			light.second->updateState(force);
			if(!_dominantLight){ //if we do not have a dominant light registered, search for one
				if(light.second->getLightMode() == LIGHT_MODE_DOMINANT){
					//setting a light as dominant, will automatically inform the lightmanager, but just in case, make sure
					_dominantLight = light.second;
				}
			}
		}
	}
}

void LightManager::generateShadowMaps(SceneRenderState* renderState){
	//Stop if we have shadows disabled
	if(!_shadowMapsEnabled) return;
	_lightProjectionMatricesCache.resize(MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
	//Tell the engine that we are drawing to depth maps
	//Remember the previous render stage type
	RenderStage prev = GFX_DEVICE.getRenderStage();
	//set the current render stage to SHADOW_STAGE
	GFX_DEVICE.setRenderStage(SHADOW_STAGE);
	//tell the rendering API to render geometry for depth storage
	GFX_DEVICE.toggleDepthMapRendering(true);
	// if we have a dominant light, generate only it's shadows
	if(_dominantLight){
		_dominantLight->generateShadowMaps(renderState);
	}else{
		//generate shadowmaps for each light
		for_each(LightMap::value_type& light, _lights){
			light.second->generateShadowMaps(renderState);
		}
	}
	//Set normal rendering settings
	GFX_DEVICE.toggleDepthMapRendering(false);
	//Revert back to the previous stage
	GFX_DEVICE.setRenderStage(prev);
}

void LightManager::previewShadowMaps(Light* light){
	//Stop if we have shadows disabled
	if(!_shadowMapsEnabled) return;
	if(_previewShadowMaps && GFX_DEVICE.getRenderStage() != DEFERRED_STAGE){
		Light* localLight = light;
		if(_dominantLight){
			localLight = _dominantLight;
		}else{
			if(localLight == NULL){
				localLight = _lights[0];
			}
		}
		localLight->getShadowMapInfo()->getShadowMap()->previewShadowMaps();
	}
}

vectorImpl<I32> LightManager::getDepthMapResolution() {
	vectorImpl<I32 > shadowMapResolution;
	shadowMapResolution.reserve(MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
	U8 counter = 0;
	for_each(Light* light, _currLightsPerNode){
		if(++counter > MAX_SHADOW_CASTING_LIGHTS_PER_NODE) break;
		shadowMapResolution.push_back(light->getShadowMapInfo()->getShadowMap()->resolution());
	}
	return shadowMapResolution;
}

//If we have computed shadowmaps, bind them before rendering any geometry;
//Always bind shadowmaps to slots 8, 9, 10, 11, 12, 13;
void LightManager::bindDepthMaps(Light* light, U8 lightIndex, U8 offset, bool overrideDominant){
	//Skip applying shadows if we are rendering to depth map, or we have shadows disabled
	if(!_shadowMapsEnabled) return;
	
	Light* lightLocal = light;
	///If we have a dominant light, then both shadow casting lights are the same = the dominant one
	///Shadow map binding has a failsafe check for this, so it's ok to call bind twice
	if(_dominantLight && !overrideDominant){
		lightLocal = _dominantLight;
	}

	if(lightLocal->getLightType() == LIGHT_TYPE_DIRECTIONAL){
		offset = (U8)_shadowArrayOffset;
	}
	if(lightLocal->getLightType() == LIGHT_TYPE_POINT){
		offset = (U8)_shadowCubeOffset;
	}
	_lightProjectionMatricesCache[lightIndex] = lightLocal->getLightProjectionMatrix();
	lightLocal->getShadowMapInfo()->getShadowMap()->Bind(offset);
}


void LightManager::unbindDepthMaps(Light* light, U8 offset, bool overrideDominant){
	if(!_shadowMapsEnabled) return;
	Light* lightLocal = light;
	if(_dominantLight && !overrideDominant){
		lightLocal = _dominantLight;
	}
	if(lightLocal->getLightType() == LIGHT_TYPE_DIRECTIONAL){
		offset = (U8)_shadowArrayOffset;
	}
	if(lightLocal->getLightType() == LIGHT_TYPE_POINT){
		offset = (U8)_shadowCubeOffset;
	}
	lightLocal->getShadowMapInfo()->getShadowMap()->Unbind(offset);
}

bool LightManager::shadowMappingEnabled(){
	if(!_shadowMapsEnabled) return false;

	for_each(LightMap::value_type& light, _lights){
		ShadowMap* sm = light.second->getShadowMapInfo()->getShadowMap();
		if(!sm) continue; ///<no shadow info;
		if(sm->getDepthMap()) return true;
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
	vectorImpl<Light* > tempLights;
	tempLights.reserve(_lights.size());
	_currLightsPerNode.resize(0);
	_currLightTypes.resize(0);

	for_each(LightMap::value_type& lightIt, _lights){
		Light* light = lightIt.second;
		if(!light->getEnabled()) continue;
		F32 luminace = 0.0f;
		F32 dist = 0.0f;
		F32 weight = 1.0f; ///<later
		if ( light->getLightType() != LIGHT_TYPE_DIRECTIONAL )  {
			// Get the luminosity.
			luminace = vec3<F32>(light->getVProperty(LIGHT_PROPERTY_DIFFUSE)).dot(lumDot);
			luminace *= light->getFProperty(LIGHT_PROPERTY_BRIGHTNESS);

			// Get the distance to the light... score it 1 to 0 near to far.
			vec3<F32> len = node->getBoundingBox().getCenter();
			len -= vec3<F32>(light->getPosition());
			F32 lenSq = len.lengthSquared();
	        F32 radiusSq = squared( light->getFProperty(LIGHT_PROPERTY_RANGE) + node->getBoundingSphere().getRadius());
            F32 distSq = radiusSq - lenSq;
         
         if ( distSq > 0.0f )
			 dist = distSq /( 1000.0f * 1000.0f );
             CLAMP<F32>(dist,0.0f, 1.0f );
		}else{///directional
			dist = 1.0f;
            luminace = 1.0f;
		}
		light->setScore( luminace * weight * dist );
		///Use type filter
		if(typeFilter != LIGHT_TYPE_PLACEHOLDER && light->getLightType() == typeFilter){
			tempLights.push_back(light);
		}
	}
	std::sort(tempLights.begin(), tempLights.end(), scoreCmpFnc());
	I32 maxLights = tempLights.size();
	CLAMP<I32>(maxLights, 0, MAX_LIGHTS_PER_SCENE_NODE);
	for(U8 i = 0; i < maxLights; i++){
		_currLightsPerNode.push_back(tempLights[i]);
		_currLightTypes.push_back(tempLights[i]->getLightType());
	}
	return maxLights;
}
