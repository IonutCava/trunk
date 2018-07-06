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
                                                   _lightSGN(NULL),
                                                   _impostor(NULL),
                                                   _id(0),
                                                   _impostorSGN(NULL),
                                                   _castsShadows(true),
                                                   _par(ParamHandler::getInstance()),
                                                   _shadowMapInfo(NULL),
                                                   _score(0)
{
    //All lights default to fully dynamic for now.
    setLightMode(LIGHT_MODE_MOVABLE);

    _properties._ambient = vec4<F32>(0.1f,0.1f,0.1f,1.0f);
    _properties._diffuse = WHITE();
    _properties._specular = WHITE();
    _properties._spotExponent;
    _properties._spotCutoff;
    _properties._attenuation = vec4<F32>(1.0f, 0.1f, 0.0f, range); //constAtt, linearAtt, quadAtt, range
    _properties._brightness;
    _properties._padding = 1.0f;
    setShadowMappingCallback(SCENE_GRAPH_UPDATE(GET_ACTIVE_SCENEGRAPH()));
    setRange(1);//<Default range of 1
    _dirty = true;
    _enabled = true;
}

Light::~Light()
{
}

bool Light::unload(){
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

void Light::updateState(){
    if(!_dirty)
        return;

    if(_drawImpostor){
        Sphere3D* lightDummy = NULL;
        if(!_impostor){
            _impostor = New Impostor(_name,_properties._attenuation.w);
            lightDummy = _impostor->getDummy();
            lightDummy->getSceneNodeRenderState().setDrawState(false);
            _impostorSGN = _lightSGN->addNode(lightDummy);
            _impostorSGN->setActive(false);
        }

        lightDummy = _impostor->getDummy();
        _lightSGN->getTransform()->setPosition(_properties._position);
        lightDummy->getMaterial()->setDiffuse(getDiffuseColor());
        lightDummy->getMaterial()->setAmbient(getDiffuseColor());
        _impostorSGN->getTransform()->setPosition(_properties._position);
        //Updating impostor range is expensive, so check if we need to
        F32 range = _properties._attenuation.w;

        if(!FLOAT_COMPARE(range, lightDummy->getRadius()))
            _impostor->getDummy()->setRadius(range);
    }

    //Do not set GL lights for deferred rendering
    if(GFX_DEVICE.getRenderer()->getType() == RENDERER_FORWARD)
        GFX_DEVICE.setLight(this);

    _dirty = false;
}

void Light::setLightProperties(const LightPropertiesV& key, const vec4<F32>& value){
    ///Light properties are very dependent on scene state. Some lights can't be changed on runtime!
    if(!GET_ACTIVE_SCENE()->state().getRunningState())
        return;

    ///Simple light's can't be changed. Period!
    if(_mode == LIGHT_MODE_SIMPLE){
        ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Simple",LightEnum(key));
        return;
    }

    ///Movable lights have no restrictions
    switch(key){
        default: ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY_SET"),_id);	return;
        case LIGHT_PROPERTY_DIFFUSE  : 	_properties._diffuse = value;		break;
        case LIGHT_PROPERTY_AMBIENT  :	_properties._ambient = value;		break;
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
    _properties._position = vec4<F32>(newPosition,_properties._position.w);
    _dirty = true;
}

void Light::setDirection(const vec3<F32>& newDirection){
    ///Togglable lights can't be moved.
    if(_mode == LIGHT_MODE_TOGGLABLE){
        ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Togglable","LIGHT_DIRECTION");
        return;
    }
    _properties._direction = vec4<F32>(newDirection,1.0f);
    _dirty = true;
}

void Light::setLightProperties(const LightPropertiesF& key, F32 value){
    //Light properties are very dependent on scene state. Some lights can't be changed on runtime!
    if(! GET_ACTIVE_SCENE()->state().getRunningState())
        return;

    if(key == LIGHT_PROPERTY_RANGE){
        setRange(value);
        return;
    }

    //Simple light's can't be changed. Period!
    if(_mode == LIGHT_MODE_SIMPLE){
        ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Simple",LightEnum(key));
        return;
    }

    //Togglable lights can't be moved.
    if(_mode == LIGHT_MODE_TOGGLABLE){
        if(key == LIGHT_PROPERTY_RANGE || key == LIGHT_PROPERTY_BRIGHTNESS ||
           key == LIGHT_PROPERTY_SPOT_EXPONENT || key == LIGHT_PROPERTY_SPOT_CUTOFF){
            ERROR_FN(Locale::get("WARNING_ILLEGAL_PROPERTY"),_id, "Light_Togglable",LightEnum(key));
            return;
        }
    }

    switch(key){
        default: ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY_SET"),_id); return;
        case LIGHT_PROPERTY_SPOT_EXPONENT : _properties._spotExponent = value; break;
        case LIGHT_PROPERTY_SPOT_CUTOFF   : _properties._spotCutoff = value; break;
        case LIGHT_PROPERTY_CONST_ATT     : _properties._attenuation.x = value; break;
        case LIGHT_PROPERTY_LIN_ATT       : _properties._attenuation.y = value; break;
        case LIGHT_PROPERTY_QUAD_ATT      : _properties._attenuation.z = value; break;
        case LIGHT_PROPERTY_BRIGHTNESS    : _properties._brightness = value; break;
    };

    _dirty = true;
}

const vec4<F32>& Light::getVProperty(const LightPropertiesV& key) const {
    switch(key){
        default: ERROR_FN(Locale::get("WARNING_INVALID_PROPERTY_GET"),_id); break;
        case LIGHT_PROPERTY_DIFFUSE  : return _properties._diffuse;
        case LIGHT_PROPERTY_AMBIENT  : return _properties._ambient;
        case LIGHT_PROPERTY_SPECULAR : return _properties._specular;
    };
    return _properties._diffuse;
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
        case LIGHT_PROPERTY_SPOT_EXPONENT : return _properties._spotExponent;
        case LIGHT_PROPERTY_SPOT_CUTOFF   : return _properties._spotCutoff;
        case LIGHT_PROPERTY_CONST_ATT     : return _properties._attenuation.x;
        case LIGHT_PROPERTY_LIN_ATT       : return _properties._attenuation.y;
        case LIGHT_PROPERTY_QUAD_ATT      : return _properties._attenuation.z;
        case LIGHT_PROPERTY_RANGE         : return _properties._attenuation.w;
        case LIGHT_PROPERTY_BRIGHTNESS    : return _properties._brightness;
    };

    return -1.0f;
}

void Light::updateBBatCurrentFrame(SceneGraphNode* const sgn){
    ///Check if range changed
    if(getRange() != sgn->getBoundingBox().getMax().x){
        sgn->getBoundingBox().setComputed(false);
    }
    return SceneNode::updateBBatCurrentFrame(sgn);
}

bool Light::computeBoundingBox(SceneGraphNode* const sgn){
    if(sgn->getBoundingBox().isComputed())
        return true;

    F32 range = getRange() * 0.5f; //diameter to radius
    sgn->getBoundingBox().set(vec3<F32>(-range), vec3<F32>(range));
    return SceneNode::computeBoundingBox(sgn);
}

bool Light::isInView(const bool distanceCheck,const BoundingBox& boundingBox,const BoundingSphere& sphere){
    return ((_impostorSGN != NULL) && _drawImpostor);
}

void Light::render(SceneGraphNode* const sgn){
    ///The isInView call should stop impostor rendering if needed
    if(!_impostor)
        return;

    _impostor->render(_impostorSGN);
}

void  Light::setRange(F32 range) {
    _dirty = true;
    _properties._attenuation.w = range;
    if(_impostor)
        _impostor->setRadius(range);
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
    //get the MVP from the new Frustum as this is the light's full MVP
    //mat4<F32> bias;
    GFX_DEVICE.getMatrix(MVP_MATRIX,_lightProjectionMatrix);
    //gfx.getMatrix(BIAS_MATRIX,bias);
    //_lightProjectionMatrix = bias * _lightProjectionMatrix;
}

void Light::generateShadowMaps(const SceneRenderState& sceneRenderState){
    ShadowMap* sm = _shadowMapInfo->getOrCreateShadowMap(sceneRenderState);

    if(!sm)
        return;

    sm->render(sceneRenderState, _callback);
}

void Light::setShadowMappingCallback(boost::function0<void> callback) {
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
        case LIGHT_PROPERTY_AMBIENT:   return "LIGHT_AMBIENT_COLOR";
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
        case LIGHT_PROPERTY_RANGE:         return "LIGHT_RANGE";
    };
    return "";
}