#include "SceneNode.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Managers/ResourceManager.h"

SceneNode::SceneNode(const SceneNode& old) : _boundingBox(old._boundingBox),
											_renderState(old._renderState),
											_noDefaultMaterial(old._noDefaultMaterial){
	if(old._transform)	_transform = New Transform(*old._transform);
	ResourceDescriptor oldMaterialDescriptor(old._material->getName());
	_material = ResourceManager::getInstance().LoadResource<Material>(oldMaterialDescriptor);
}

void SceneNode::drawBBox(){
	_boundingBox.setVisibility(SceneManager::getInstance().getActiveScene()->drawBBox());

	if(_boundingBox.getVisibility())
		GFXDevice::getInstance().drawBox3D(_boundingBox.getMin(),_boundingBox.getMax());
}

Transform*	const SceneNode::getTransform(){
	
	if(!_transform){
		Quaternion rotation; rotation.FromEuler(0,0,-1);
		_transform = New Transform(rotation, vec3(0,0,0),vec3(1,1,1));
	}
	assert(_transform);
	return _transform;
}

void SceneNode::setTransform(Transform* t){
	if(_transform != NULL){
		delete _transform;
	}
	_transform = new Transform(*t);
}

	
	
Material* SceneNode::getMaterial(){
	if(_material == NULL){
		if(!_noDefaultMaterial){
			ResourceDescriptor defaultMat("defaultMaterial");
			_material = ResourceManager::getInstance().LoadResource<Material>(defaultMat);
		}
	}
	return _material;
}


void SceneNode::setMaterial(Material* m){
	if(_material != NULL) 
		ResourceManager::getInstance().remove(_material);
	_material = m;
}

void SceneNode::clearMaterials(){
	if(_material != NULL){
		ResourceManager::getInstance().remove(_material);
	}
}

inline	BoundingBox&	SceneNode::getBoundingBox() {
	if(!_boundingBox.isComputed())
		computeBoundingBox();
	_boundingBox.Transform(_originalBB,getTransform()->getMatrix());
	return _boundingBox;
}

bool SceneNode::unload(){
	if(_transform != NULL) {
		delete _transform;
		_transform = NULL;
	} 
	clearMaterials();

	return true;
}