#include "Object3D.h"
#include "Managers/SceneManager.h"
#include "Managers/ResourceManager.h"
Object3D::Object3D(const Object3D& old) : Resource(old),_material(old._material), _selected(old._selected), _update(old._update),
										  _bb(old._bb), _originalBB(old._originalBB),_geometryType(old._geometryType),
										  _drawBB(old._drawBB),_render(old._render),_computedLightShaders(old._computedLightShaders)
{
	if(old._transform)	_transform = New Transform(*old._transform);
	for(std::vector<Shader*>::const_iterator it = old._shaders.begin(); it != old._shaders.end(); it++)
		_shaders.push_back(*it);
}

Object3D::~Object3D() {	

}

void Object3D::addChild(Object3D* object){
	//Warning: use this as follows: blabla->addChild(ResourceManager::getInstance().LoadResource<Object3D>("blabla_Child");
	//         so that the entire object hierrachy can be destroyed correctly
	_children.push_back(object);
}

void Object3D::onDraw(){
	if(!_computedLightShaders){
		Shader* s;
		if(!_material.textures.empty())
			s = ResourceManager::getInstance().LoadResource<Shader>("DeferredShadingPass1");
		else
			s = ResourceManager::getInstance().LoadResource<Shader>("DeferredShadingPass1_color.frag,DeferredShadingPass1.vert");
		
		_shaders.push_back(s);
		_computedLightShaders = true;
	}
	if(!_bb.isComputed()) computeBoundingBox();
	_bb.Transform(_originalBB, getTransform()->getMatrix());
	drawBBox();
}

void Object3D::drawBBox(){
	_bb.setVisibility(SceneManager::getInstance().getActiveScene()->drawBBox());

	if(_bb.getVisibility())
		GFXDevice::getInstance().drawBox3D(_bb.getMin(),_bb.getMax());
}

Transform*	const Object3D::getTransform(){
	if(!_transform){
		Quaternion rotation; rotation.FromEuler(0,0,-1);
		_transform = New Transform(rotation, vec3(0,0,0),vec3(1,1,1));
	}

	return _transform;
}

bool Object3D::unload(){
	if(_computedLightShaders)
		clearMaterials();
	return true;
	if(_transform) {
		delete _transform;
		_transform = NULL;
	} 
	if(!_children.empty()){
		for(U8 i = 0; i < _children.size(); i++){
			if(_children[i]){
				ResourceManager::getInstance().remove(_children[i]);
			}
		}
	}
}

void Object3D::clearMaterials(){
	if(!getShaders().empty()){
		for(U8 i = 0; i < getShaders().size(); i++)
			ResourceManager::getInstance().remove(getShaders()[i]);
		getShaders().clear();
	}
	_computedLightShaders = true;
}