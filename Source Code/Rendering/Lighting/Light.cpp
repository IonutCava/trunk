#include "Headers/Light.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

Light::Light(const U8 slot,const F32 range,const LightType& type) :
                                                   SceneNode(TYPE_LIGHT),
                                                   _type(type),
                                                   _slot(slot),
                                                   _drawImpostor(false),
                                                   _updateLightBB(false),
                                                   _lightSGN(NULL),
                                                   _impostor(NULL),
                                                   _id(0),
                                                   _impostorSGN(NULL),
                                                   _castsShadows(true),
                                                   _par(ParamHandler::getInstance()),
                                                   _shadowMapInfo(NULL),
                                                   _score(0.0f)
{
    //All lights default to fully dynamic for now.
    setLightMode(LIGHT_MODE_MOVABLE);

    _properties._diffuse.set(DefaultColors::WHITE());
    _properties._specular.set(DefaultColors::WHITE());
    _properties._direction.w = 0.0f;
   
    _properties._attenuation = vec4<F32>(1.0f, 0.07f, 0.017f, 45.0f); //constAtt, linearAtt, quadAtt, spotCuroff
    _properties._specular.w = 1.0f;
    setShadowMappingCallback(DELEGATE_BIND(&SceneManager::renderVisibleNodes,
                                           DELEGATE_REF(SceneManager::getInstance())));
    _dirty = true;
    _enabled = true;
    _bias.bias();

    assert(LightManager::hasInstance());
    LightManager::getInstance().addLight(this);
}

Light::~Light()
{
    unload();
}

bool Light::unload(){
    if(getState() != RES_LOADED && getState() != RES_LOADING)
        return true;

    if(_impostor){
        _lightSGN->removeNode(_impostorSGN);
        SAFE_DELETE(_impostor);
    }

    assert(LightManager::hasInstance());
    LightManager::getInstance().removeLight(getId());
        
    removeShadowMapInfo();

    return SceneNode::unload();
}

void Light::postLoad(SceneGraphNode* const sgn) {
    //Hold a pointer to the light's location in the SceneGraph
    _lightSGN = sgn;
    _updateLightBB = true;
}

void Light::updateState(const bool force){
    assert(_lightSGN != NULL);

    if(force) _dirty = true;

    if(_type != LIGHT_TYPE_DIRECTIONAL) {

        if(_mode == LIGHT_MODE_MOVABLE) {
            _properties._position.set(_lightSGN->getTransform()->getPosition());
            _dirty = true;
        }

        if(!_dirty && !_updateLightBB) return;

        if(_mode != LIGHT_MODE_MOVABLE)
            _lightSGN->getTransform()->setPosition(_properties._position);

        if(_drawImpostor){
            Sphere3D* lightDummy = NULL;
            if(!_impostor){
                _impostor = New Impostor(_name,_properties._specular.w);
                lightDummy = _impostor->getDummy();
                lightDummy->getSceneNodeRenderState().setDrawState(false);
                _impostorSGN = _lightSGN->addNode(lightDummy);
                _impostorSGN->setActive(false);
            }

            lightDummy = _impostor->getDummy();

            lightDummy->getMaterial()->setDiffuse(getDiffuseColor());
            lightDummy->getMaterial()->setAmbient(getDiffuseColor());

            //Updating impostor range is expensive, so check if we need to
            F32 range = _properties._specular.w;

            if(!FLOAT_COMPARE(range, lightDummy->getRadius()))
                _impostor->getDummy()->setRadius(range);
        }

        if(_updateLightBB){
            _lightSGN->updateBoundingBoxTransform(_lightSGN->getTransform()->getGlobalMatrix());
            _updateLightBB = false;
        }
    }

    //Do not set API lights for deferred rendering
    if(getEnabled() && GFX_DEVICE.getRenderer()->getType() == RENDERER_FORWARD && _dirty)
        GFX_DEVICE.setLight(this);

    _dirty = false;
}

void Light::setLightProperties(const LightPropertiesV& key, const vec3<F32>& value){
    ///Simple light's can't be changed. Period!
    if(_mode == LIGHT_MODE_SIMPLE){
        ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Simple",LightEnum(key));
        return;
    }

    ///Movable lights have no restrictions
    switch(key){
        default: ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY_SET"),_id);	return;
        case LIGHT_PROPERTY_DIFFUSE  : 	_properties._diffuse = value;		break;
        case LIGHT_PROPERTY_SPECULAR :	_properties._specular = value;		break;
    };

    _dirty = true;
}

void Light::setPosition(const vec3<F32>& newPosition){
    ///Togglable lights can't be moved.
    if(_mode == LIGHT_MODE_TOGGLABLE){
        ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Togglable","LIGHT_POSITION");
        return;
    }
    _properties._position = vec4<F32>(newPosition, _properties._position.w);

    if(_mode == LIGHT_MODE_MOVABLE)
        _lightSGN->getTransform()->setPosition(newPosition);

    if(_type != LIGHT_TYPE_DIRECTIONAL)
        _updateLightBB = true;

    _dirty = true;
}

void Light::setDirection(const vec3<F32>& newDirection){
    ///Togglable lights can't be moved.
    if(_mode == LIGHT_MODE_TOGGLABLE){
        ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Togglable","LIGHT_DIRECTION");
        return;
    }
    if(_type == LIGHT_TYPE_SPOT)
        _properties._direction = vec4<F32>(newDirection,1.0f);
    else
        _properties._position = vec4<F32>(newDirection, _type == LIGHT_TYPE_DIRECTIONAL ? 0.0f : 1.0f);

    _dirty = true;

    if(_type != LIGHT_TYPE_DIRECTIONAL)
        _updateLightBB = true;
}

void Light::setLightProperties(const LightPropertiesF& key, F32 value){
    //Simple light's can't be changed. Period!
    if(_mode == LIGHT_MODE_SIMPLE){
        ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Simple",LightEnum(key));
        return;
    }

    //Togglable lights can't be moved.
    if(_mode == LIGHT_MODE_TOGGLABLE){
        if(key == LIGHT_PROPERTY_BRIGHTNESS || key == LIGHT_PROPERTY_SPOT_EXPONENT || key == LIGHT_PROPERTY_SPOT_CUTOFF){
            ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Togglable",LightEnum(key));
            return;
        }
    }

    switch(key){
        default: ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY_SET"),_id); return;
        case LIGHT_PROPERTY_SPOT_EXPONENT : _properties._direction.w = value; break;
        case LIGHT_PROPERTY_SPOT_CUTOFF   : _properties._attenuation.w = value; break;
        case LIGHT_PROPERTY_CONST_ATT     : _properties._attenuation.x = value; break;
        case LIGHT_PROPERTY_LIN_ATT       : _properties._attenuation.y = value; break;
        case LIGHT_PROPERTY_QUAD_ATT      : _properties._attenuation.z = value; break;
        case LIGHT_PROPERTY_BRIGHTNESS    : {
            _properties._specular.w = value; 
            if(_impostor)
                _impostor->setRadius(value);
        }break;
        case LIGHT_PROPERTY_AMBIENT       :	_properties._diffuse.w = value;	break;
    };

    _dirty = true;
}

vec3<F32> Light::getVProperty(const LightPropertiesV& key) const {
    switch(key){
        default: ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY_GET"),_id); break;
        case LIGHT_PROPERTY_DIFFUSE  : return _properties._diffuse.rgb();
        case LIGHT_PROPERTY_SPECULAR : return _properties._specular.rgb();
    };
    return _properties._diffuse.rgb();
}

///Get light floating point properties
F32 Light::getFProperty(const LightPropertiesF& key) const {
    //spot values are only for spot lights
    if(key == LIGHT_PROPERTY_SPOT_EXPONENT || key == LIGHT_PROPERTY_SPOT_CUTOFF && _type != LIGHT_TYPE_SPOT){
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_FLOAT_PROPERTY_REQUEST"),  _id);
        return -1.0f;
    }

    switch(key){
        default: ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY_GET"),_id); break;
        case LIGHT_PROPERTY_CONST_ATT     : return _properties._attenuation.x;
        case LIGHT_PROPERTY_LIN_ATT       : return _properties._attenuation.y;
        case LIGHT_PROPERTY_QUAD_ATT      : return _properties._attenuation.z;
        case LIGHT_PROPERTY_SPOT_CUTOFF   : return _properties._attenuation.w;
        case LIGHT_PROPERTY_SPOT_EXPONENT : return _properties._direction.w;
        case LIGHT_PROPERTY_BRIGHTNESS    : return _properties._specular.w;
        case LIGHT_PROPERTY_AMBIENT       : return _properties._diffuse.w;
    };

    return -1.0f;
}

void Light::updateBBatCurrentFrame(SceneGraphNode* const sgn){
    if(_type == LIGHT_TYPE_DIRECTIONAL) return;

    // Check if range changed
    if(FLOAT_COMPARE(getFProperty(LIGHT_PROPERTY_BRIGHTNESS) * 0.5f, sgn->getBoundingBox().getMax().x))
        return;
    
    sgn->getBoundingBox().setComputed(false);
    
    _updateLightBB = true;

    return SceneNode::updateBBatCurrentFrame(sgn);
}

bool Light::computeBoundingBox(SceneGraphNode* const sgn){
    if(_type == LIGHT_TYPE_DIRECTIONAL){
        vec4<F32> directionalLightPosition = _properties._position * Config::DIRECTIONAL_LIGHT_DISTANCE * -1.0f;

        sgn->getBoundingBox().set(directionalLightPosition.xyz() - vec3<F32>(10), 
                                  directionalLightPosition.xyz() + vec3<F32>(10));
    }else{
        F32 range = getFProperty(LIGHT_PROPERTY_BRIGHTNESS) * 0.5f; //diameter to radius
        sgn->getBoundingBox().set(vec3<F32>(-range), vec3<F32>(range));
    }

    _updateLightBB = true;

    return SceneNode::computeBoundingBox(sgn);
}

bool Light::isInView(const BoundingBox& boundingBox,const BoundingSphere& sphere, const bool distanceCheck){
    return ((_impostorSGN != NULL) && _drawImpostor);
}

void Light::render(SceneGraphNode* const sgn){
    // The isInView call should stop impostor rendering if needed
    if(!_impostor)  return;

    _impostor->render(sgn);
}

void Light::addShadowMapInfo(ShadowMapInfo* shadowMapInfo){
    SAFE_UPDATE(_shadowMapInfo, shadowMapInfo);
}

bool Light::removeShadowMapInfo(){
    SAFE_DELETE(_shadowMapInfo);
    return true;
}

void Light::setCameraToSceneView(){
    //Set the ortho projection so that it encompasses the entire scene
    GFX_DEVICE.setOrthoProjection(vec4<F32>(-1.0, 1.0, -1.0, 1.0), _zPlanes);
    //Extract the view frustum from this projection mode
    Frustum::getInstance().Extract(_eyePos - _lightPos);
    //get the VP from the new Frustum as this is the light's full MVP
    GFX_DEVICE.getMatrix(VIEW_PROJECTION_MATRIX, _lightProjectionMatrix);
    //_lightProjectionMatrix.set(_bias * _lightProjectionMatrix);
}

void Light::generateShadowMaps(const SceneRenderState& sceneRenderState){
    ShadowMap* sm = _shadowMapInfo->getOrCreateShadowMap(sceneRenderState);

    if(!sm)
        return;

    sm->render(sceneRenderState, _callback);
    sm->postRender();
}

void Light::setShadowMappingCallback(const DELEGATE_CBK& callback) {
    _callback = callback;
}

void Light::setLightMode(const LightMode& mode) {
    _mode = mode;
    //if the light became dominant, inform the lightmanager
    if(mode == LIGHT_MODE_DOMINANT)
        LightManager::getInstance().setDominantLight(this);
}

const char* Light::LightEnum(const LightPropertiesV& key) const {
    switch(key){
        case LIGHT_PROPERTY_DIFFUSE:   return "LIGHT_DIFFUSE_COLOR";
        case LIGHT_PROPERTY_SPECULAR:  return "LIGHT_SPECULAT_COLOR";
    };
    return "";
}

const char* Light::LightEnum(const LightPropertiesF& key) const {
    switch(key){
        case LIGHT_PROPERTY_SPOT_EXPONENT: return "LIGHT_SPOT_EXPONENT";
        case LIGHT_PROPERTY_SPOT_CUTOFF:   return "LIGHT_SPOT_CUTOFF";
        case LIGHT_PROPERTY_CONST_ATT:     return "LIGHT_CONST_ATTENUATION";
        case LIGHT_PROPERTY_LIN_ATT:       return "LIGHT_LINEAR_ATTENUATION";
        case LIGHT_PROPERTY_QUAD_ATT:      return "LIGHT_QUADRATIC_ATTENUATION";
        case LIGHT_PROPERTY_BRIGHTNESS:    return "LIGHT_BRIGHTNESS";
        case LIGHT_PROPERTY_AMBIENT:       return "LIGHT_AMBIENT_COLOR";
    };
    return "";
}