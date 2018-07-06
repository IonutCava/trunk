#include "Headers/Light.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Managers/Headers/SceneManager.h"

Light::Light(U8 slot, F32 radius) : SceneNode(TYPE_LIGHT),    _slot(slot), 
													_radius(radius), _drawImpostor(false),
													_lightSGN(NULL), _impostor(NULL),
													_id(0),			 _impostorSGN(NULL),
													_castsShadows(true),_resolutionFactor(1)
{
	vec4 _white(1.0f,1.0f,1.0f,1.0f);
	vec2 angle = vec2(0.0f, RADIANS(45.0f));
	vec4 position = vec4(-cosf(angle.x) * sinf(angle.y),-cosf(angle.y),	-sinf(angle.x) * sinf(angle.y),	0.0f );
	vec4 diffuse = _white.lerp(vec4(1.0f, 0.5f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + cosf(angle.y) * 0.75f);
	
	_lightProperties_v[LIGHT_POSITION] = position;
	_lightProperties_v[LIGHT_AMBIENT] = vec4(0.1f,0.1f,0.1f,1.0f);
	_lightProperties_v[LIGHT_DIFFUSE] = diffuse;
	_lightProperties_v[LIGHT_SPECULAR] = diffuse;
	_lightProperties_v[LIGHT_SPOT_DIRECTION] = vec4(0,0,0,0);
	_type = LIGHT_DIRECTIONAL;

	_lightProperties_f[LIGHT_SPOT_EXPONENT] = 1;
	_lightProperties_f[LIGHT_SPOT_CUTOFF] = 1;
	_lightProperties_f[LIGHT_CONST_ATT] = 1;
	_lightProperties_f[LIGHT_LIN_ATT] = 0.1f;
	_lightProperties_f[LIGHT_QUAD_ATT] = 0.0f;

	///Shadow Mapping disabled for deferred renderer
	if(!GFX_DEVICE.getDeferredRendering()){
		SceneGraph* sg = SceneManager::getInstance().getActiveScene()->getSceneGraph();
		setShadowMappingCallback(boost::bind(&SceneGraph::render, sg));
	}
	_dirty = true;
	_enabled = true;
}

Light::~Light(){
	for_each(FrameBufferObject* dm, _depthMaps){
		SAFE_DELETE(dm);
	}
	_depthMaps.clear();
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

bool Light::load(const std::string& name){
	setName(name);
	return true;
}

void Light::postLoad(SceneGraphNode* const sgn) {
	//Hold a pointer to the light's location in the SceneGraph
	_lightSGN = sgn;
}	

void Light::updateState(bool force){
	if(_dirty || force){
		_lightSGN->getTransform()->setPosition(_lightProperties_v[LIGHT_POSITION]);
		if(_drawImpostor && _impostor){
			_impostor->getDummy()->getMaterial()->setDiffuse(getDiffuseColor());
			_impostor->getDummy()->getMaterial()->setAmbient(getDiffuseColor());
			_impostorSGN->getTransform()->setPosition(_lightProperties_v[LIGHT_POSITION]);
			///Updating impostor radius is expensive, so check if we need to
			if(!FLOAT_COMPARE(_radius,_impostor->getDummy()->getRadius())){
				_impostor->getDummy()->setRadius(_radius);
			}
		}

		///Do not set GL lights for deferred rendering
		if(!GFX_DEVICE.getDeferredRendering()){
			GFX_DEVICE.setLight(this);
		}
		_dirty = false;
	}
}

void Light::setLightProperties(const LIGHT_V_PROPERTIES& propName, const vec4& value){
	if (_lightProperties_v.find(propName) != _lightProperties_v.end()){
		_lightProperties_v[propName] = value;
		_dirty = true;
	}
}

void Light::setLightProperties(const LIGHT_F_PROPERTIES& propName, F32 value){
	if (_lightProperties_f.find(propName) != _lightProperties_f.end()){
		_lightProperties_f[propName] = value;
		_dirty = true;
	}
}

void Light::render(SceneGraphNode* const sgn){
	///The isInView call should stop impostor rendering if needed
	if(!_impostor){
		_impostor = New Impostor(_name,_radius);	
		_impostorSGN = _lightSGN->addNode(_impostor->getDummy()); 
	}
	_impostor->render(_impostorSGN);
}

void  Light::setRadius(F32 radius) {
	_dirty = true;
	_radius = radius;
	if(_impostor){
		_impostor->setRadius(radius);
	}
}