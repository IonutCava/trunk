#include "Headers/Light.h"
#include "Headers/LightImpostor.h"

#include "Hardware/Video/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/ResourceManager.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Core/Headers/Application.h"

Light::Light(U8 slot, F32 radius) : SceneNode(),    _slot(slot), 
													_radius(radius), _drawImpostor(false),
													_lightSGN(NULL), _impostor(NULL),
													_id(0),			 _impostorSGN(NULL),
													_castsShadows(true)
{
	vec4 _white(1.0f,1.0f,1.0f,1.0f);
	vec2 angle = vec2(0.0f, RADIANS(45.0f));
	vec4 position = vec4(-cosf(angle.x) * sinf(angle.y),-cosf(angle.y),	-sinf(angle.x) * sinf(angle.y),	0.0f );
	vec4 diffuse = _white.lerp(vec4(1.0f, 0.5f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + cosf(angle.y) * 0.75f);
	
	_lightProperties_v["position"] = position;
	_lightProperties_v["ambient"] = vec4(0.1f,0.1f,0.1f,1.0f);
	_lightProperties_v["diffuse"] = diffuse;
	_lightProperties_v["specular"] = diffuse;
	_lightProperties_v["target"] = vec4(0,0,0,0);
	_lightProperties_v["spotDirection"] = vec4(0,0,0,0);
	_type = LIGHT_DIRECTIONAL;

	_lightProperties_f["spotExponent"] = 1;
	_lightProperties_f["spotCutoff"] = 1;
	_lightProperties_f["constAtt"] = 1;
	_lightProperties_f["linearAtt"] = 1;
	_lightProperties_f["quadAtt"] = 1;
}

Light::~Light(){
	for_each(FrameBufferObject* dm, _depthMaps){
		if(dm){
			delete dm;
			dm = NULL;
		}
	}
	_depthMaps.clear();
}

bool Light::unload(){
	_lightProperties_v.clear();
	_lightProperties_f.clear();
	LightManager::getInstance().removeLight(getId());
	delete _impostor;
	_impostor = NULL;
	_lightSGN->removeNode(_impostorSGN);
	return SceneNode::unload();
}

bool Light::load(const std::string& name){
	setName(name);
	_impostor = New LightImpostor(name,_radius);	

	return true;
}

void Light::postLoad(SceneGraphNode* const node) {
	//Hold a pointer to the light's location in the SceneGraph
	_lightSGN = node;
	_impostorSGN = node->addNode(_impostor->getDummy()); 
}	

void Light::onDraw(){
	GFXDevice& gfx = GFXDevice::getInstance();
	
	_lightSGN->getTransform()->setPosition(_lightProperties_v["position"]);
	_impostor->getDummy()->getMaterial()->setDiffuse(getDiffuseColor());
	_impostor->getDummy()->getMaterial()->setAmbient(getDiffuseColor());
	gfx.setLight(_slot,_lightProperties_v,_lightProperties_f, _type);
}

void Light::setLightProperties(const std::string& name, const vec4& value){
	unordered_map<std::string,vec4>::iterator it = _lightProperties_v.find(name);
	if (it != _lightProperties_v.end())
		_lightProperties_v[name] = value;
}

void Light::setLightProperties(const std::string& name, F32 value){
	unordered_map<std::string,F32>::iterator it = _lightProperties_f.find(name);
	if (it != _lightProperties_f.end())
		_lightProperties_f[name] = value;
}

void Light::render(SceneGraphNode* const node){
	//The isInView call should stop impostor rendering if needed
	_impostor->render(node);
}