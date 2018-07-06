#include "Headers/Light.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"



Light::Light(U8 slot, F32 range, LightType type) : SceneNode(TYPE_LIGHT), 
										           _type(type),
												   _slot(slot), 
												   _drawImpostor(false),
												   _lightSGN(NULL),
												   _impostor(NULL),
												   _id(0),
												   _impostorSGN(NULL),
 												   _castsShadows(true),
												   _par(ParamHandler::getInstance()),
												   _shadowMapInfo(NULL),
												   _score(0)
{
	///All lights default to fully dynamic for now.
	setLightMode(LIGHT_MODE_MOVABLE);

	vec4<F32> diffuse = WHITE();
	
	_lightProperties_v[LIGHT_PROPERTY_AMBIENT] = vec4<F32>(0.1f,0.1f,0.1f,1.0f);
	_lightProperties_v[LIGHT_PROPERTY_DIFFUSE] = diffuse;
	_lightProperties_v[LIGHT_PROPERTY_SPECULAR] = diffuse;

	_lightProperties_f[LIGHT_PROPERTY_CONST_ATT] = 1;
	_lightProperties_f[LIGHT_PROPERTY_LIN_ATT] = 0.1f;
	_lightProperties_f[LIGHT_PROPERTY_QUAD_ATT] = 0.0f;
	_lightProperties_f[LIGHT_PROPERTY_RANGE] = range;
	setShadowMappingCallback(SCENE_GRAPH_UPDATE(GET_ACTIVE_SCENE()->getSceneGraph()));
	setRange(1);//<Default range of 1
	_dirty = true;
	_enabled = true;
}

Light::~Light()
{
}

bool Light::unload(){
	_lightProperties_v.clear();
	_lightProperties_f.clear();
	LightManager::getInstance().removeLight(getId());
	if(_impostor){
		_lightSGN->removeNode(_impostorSGN);
		SAFE_DELETE(_impostor);
	}
	return SceneNode::unload();
}

void Light::postLoad(SceneGraphNode* const sgn) {
	//Hold a pointer to the light's location in the SceneGraph
	_lightSGN = sgn;
}	

void Light::updateState(bool force){
	if(_dirty || force){
        if(_drawImpostor){
            if(!_impostor){
                _impostor = New Impostor(_name,_lightProperties_f[LIGHT_PROPERTY_RANGE]);	
                _impostor->getDummy()->getSceneNodeRenderState().setDrawState(false);
		        _impostorSGN = _lightSGN->addNode(_impostor->getDummy());
                _impostorSGN->setActive(false);
            }
			_lightSGN->getTransform()->setPosition(_position);
			_impostor->getDummy()->getMaterial()->setDiffuse(getDiffuseColor());
			_impostor->getDummy()->getMaterial()->setAmbient(getDiffuseColor());
			_impostorSGN->getTransform()->setPosition(_position);
			///Updating impostor range is expensive, so check if we need to
			F32 range = _lightProperties_f[LIGHT_PROPERTY_RANGE];
			if(!FLOAT_COMPARE(range,_impostor->getDummy()->getRadius())){
				_impostor->getDummy()->setRadius(range);
			}
		}

		///Do not set GL lights for deferred rendering
		if(!GFX_DEVICE.getDeferredRendering()){
			GFX_DEVICE.setLight(this);
		}
		_dirty = false;
	}
}

void Light::setLightProperties(LightPropertiesV key, const vec4<F32>& value){
	if (_lightProperties_v.find(key) != _lightProperties_v.end()){
		///Light properties are very dependent on scene state. Some lights can't be changed on runtime!
		if(GET_ACTIVE_SCENE()->state()->getRunningState()){
			switch(_mode){
				///Simple light's can't be changed. Period!
				case LIGHT_MODE_SIMPLE:
					ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Simple",LightEnum(key));
					return;
				///Movable lights have no restrictions
				case LIGHT_MODE_MOVABLE:
					break;
			};
		}
		
		_lightProperties_v[key] = value;
		_dirty = true;
	}else{
		ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY"),_id);
	}
}

void Light::setPosition(const vec3<F32>& newPosition){
	///Togglable lights can't be moved.
	if(_mode == LIGHT_MODE_TOGGLABLE){
		ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Togglable","LIGHT_POSITION");
		return;
	}
	_position = newPosition;
	_dirty = true;
}

void Light::setDirection(const vec3<F32>& newDirection){
	///Togglable lights can't be moved.
	if(_mode == LIGHT_MODE_TOGGLABLE){
		ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Togglable","LIGHT_DIRECTION");
		return;
	}
	_direction = newDirection;
	_dirty = true;
}

void Light::setLightProperties(LightPropertiesF key, F32 value){
	if(key == LIGHT_PROPERTY_RANGE){
		setRange(value);
		return;
	}

	if (_lightProperties_f.find(key) != _lightProperties_f.end()){	
		///Light properties are very dependent on scene state. Some lights can't be changed on runtime!
		if(GET_ACTIVE_SCENE()->state()->getRunningState()){
			switch(_mode){
				///Simple light's can't be changed. Period!
				case LIGHT_MODE_SIMPLE:
					ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Simple",LightEnum(key));
					return;
				///Togglable lights can't be moved.
				case LIGHT_MODE_TOGGLABLE:
					if(key == LIGHT_PROPERTY_RANGE || key == LIGHT_PROPERTY_BRIGHTNESS ||
					   key == LIGHT_PROPERTY_SPOT_EXPONENT || key == LIGHT_PROPERTY_SPOT_CUTOFF){ 
						ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Togglable",LightEnum(key));
						return;
					}
					break;
				///Movable lights have no restrictions
				case LIGHT_MODE_MOVABLE:
					break;
			};
		}
		_lightProperties_f[key] = value;
		_dirty = true;
	}else{
		ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY"),_id);
	}
}

///Get light floating point properties 
F32 Light::getFProperty(LightPropertiesF key){
	///spot values are only for spot lights
	if(key == LIGHT_PROPERTY_SPOT_EXPONENT || key == LIGHT_PROPERTY_SPOT_CUTOFF && _type != LIGHT_TYPE_SPOT){
		ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_FLOAT_PROPERTY_REQUEST"),  _id);
		return _lightProperties_f[LIGHT_PROPERTY_CONST_ATT];
	}
	return _lightProperties_f[key];
}

void Light::updateBBatCurrentFrame(SceneGraphNode* const sgn){
    ///Check if range changed
    if(getRange() != sgn->getBoundingBox().getMax().x){
        sgn->getBoundingBox().setComputed(false);
    }
    return SceneNode::updateBBatCurrentFrame(sgn);
}

bool Light::computeBoundingBox(SceneGraphNode* const sgn){
    if(sgn->getBoundingBox().isComputed()) return true;
    F32 range = getRange() * 0.5f; //diameter to radius
	sgn->getBoundingBox().set(vec3<F32>(-range,-range,-range),vec3<F32>(range,range,range));
	return SceneNode::computeBoundingBox(sgn);
}

bool Light::isInView(bool distanceCheck,BoundingBox& boundingBox,const BoundingSphere& sphere){
    if(!_drawImpostor) return false;
    return (_impostorSGN != NULL);
}

void Light::render(SceneGraphNode* const sgn){
	///The isInView call should stop impostor rendering if needed
	if(!_impostor) return;
	_impostor->render(_impostorSGN);
}

void  Light::setRange(F32 range) {
	_dirty = true;
	_lightProperties_f[LIGHT_PROPERTY_RANGE] = range;
	if(_impostor){
		_impostor->setRadius(range);
	}
}

void Light::addShadowMapInfo(ShadowMapInfo* shadowMapInfo){
	SAFE_UPDATE(_shadowMapInfo, shadowMapInfo);
}

bool Light::removeShadowMapInfo(){
	SAFE_DELETE(_shadowMapInfo);
	return true;
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
	_lightProjectionMatrix = frust.getModelViewProjectionMatrix();
}

void Light::generateShadowMaps(SceneRenderState* renderState){
	ShadowMap* sm = _shadowMapInfo->getOrCreateShadowMap(renderState);
	if(sm){
		sm->render(renderState, _callback);
	}
}

void Light::setShadowMappingCallback(boost::function0<void> callback) {
	_callback = callback;
}

void Light::setLightMode(LightMode mode) {
	_mode = mode;
	//if the light became dominant, inform the lightmanager
	if(mode == LIGHT_MODE_DOMINANT){
		LightManager::getInstance().setDominantLight(this);
	}
}

const char* Light::LightEnum(LightPropertiesV key){
	switch(key){
		case LIGHT_PROPERTY_DIFFUSE:   return "LIGHT_DIFFUSE_COLOR";
		case LIGHT_PROPERTY_AMBIENT:   return "LIGHT_AMBIENT_COLOR";
		case LIGHT_PROPERTY_SPECULAR:  return "LIGHT_SPECULAT_COLOR";
	};
	return "";
}

const char* Light::LightEnum(LightPropertiesF key){
	switch(key){
		case LIGHT_PROPERTY_SPOT_EXPONENT: return "LIGHT_SPOT_EXPONENT";
		case LIGHT_PROPERTY_SPOT_CUTOFF:   return "LIGHT_SPOT_CUTOFF";
		case LIGHT_PROPERTY_CONST_ATT:     return "LIGHT_CONST_ATTENUATION";
		case LIGHT_PROPERTY_LIN_ATT:       return "LIGHT_LINEAR_ATTENUATION";
		case LIGHT_PROPERTY_QUAD_ATT:      return "LIGHT_QUADRATIC_ATTENUATION";
		case LIGHT_PROPERTY_BRIGHTNESS:    return "LIGHT_BRIGHTNESS";
		case LIGHT_PROPERTY_RANGE:         return "LIGHT_RANGE";
	};
	return "";
}