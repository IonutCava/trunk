#include "Mesh.h"
#include "Managers/ResourceManager.h"
#include "Rendering/Frustum.h"
#include "Managers/SceneManager.h"
#include "Utility/Headers/Quaternion.h"
using namespace std;

Mesh::Mesh(const Mesh& old) : Object3D(old),
							  _visibleToNetwork(old._visibleToNetwork){
	
	vector<SubMesh* >::const_iterator it;
	_subMeshes.reserve(old._subMeshes.size());
	for(it = old._subMeshes.begin(); it != old._subMeshes.end(); ++it)
		_subMeshes.push_back(*it);
}

bool Mesh::load(const string& name){
	_name = name;
	return true;
}

bool Mesh::unload(){
	for(_subMeshIterator = getSubMeshes().begin(); _subMeshIterator != getSubMeshes().end();){
		if((*_subMeshIterator)->unload()){
			delete (*_subMeshIterator);
			(*_subMeshIterator) = NULL;
			_subMeshIterator = getSubMeshes().erase(_subMeshIterator);
		}
		else
			 _subMeshIterator++;
	}
	getSubMeshes().clear();
	SceneNode::unload();
	return true;
}

void Mesh::onDraw(){
	//onDraw must be called before any other rendering call (such as "isInView")
	//in order to properly compute the boundingbox
	setRenderState(SceneManager::getInstance().getActiveScene()->drawObjects());
	if(getTransform()){
		_boundingBox.Transform(getOriginalBoundingBox(),getTransform()->getMatrix()); 
	}
	drawBBox();
	if(!_computedLightShaders) {
		computeLightShaders();
	}
}

bool Mesh::getVisibility(){
	if(!_render || !isInView() || _subMeshes.empty())
		return false;
	return true;

}

void Mesh::computeLightShaders(){
	getMaterial()->computeLightShaders();
	_computedLightShaders = true;
}

bool Mesh::isInView(){
	if(!_render) return false;

	if(!GFXDevice::getInstance().getDepthMapRendering())
	{
		vec3 vEyeToChunk = _boundingBox.getCenter() - Frustum::getInstance().getEyePos();
		if(vEyeToChunk.length() > SceneManager::getInstance().getActiveScene()->getGeneralVisibility()) return false;
	}

	vec3 center = _boundingBox.getCenter();
	float radius = (_boundingBox.getMax()-center).length();

	if(!_boundingBox.ContainsPoint(Frustum::getInstance().getEyePos()))
	{
		switch(Frustum::getInstance().ContainsSphere(center, radius)) {
				case FRUSTUM_OUT: 	
					return false;
				
				case FRUSTUM_INTERSECT:	
					if(Frustum::getInstance().ContainsBoundingBox(_boundingBox) == FRUSTUM_OUT)
						return false;
				
			}
	}
	return true;
}


bool Mesh::computeBoundingBox(){
	_boundingBox.setMin(vec3(100000.0f, 100000.0f, 100000.0f));
	_boundingBox.setMax(vec3(-100000.0f, -100000.0f, -100000.0f));

	for(_subMeshIterator = _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
			_boundingBox.Add((*_subMeshIterator)->getBoundingBox());

	_boundingBox.isComputed() = true;
	setOriginalBoundingBox(_boundingBox);
	return true;
}

SubMesh*  Mesh::getSubMesh(const string& name)
{
	for(_subMeshIterator =  _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
		if((*_subMeshIterator)->getName().compare(name) == 0)
			return (*_subMeshIterator);
	return NULL;
}

bool Mesh::optimizeSubMeshes()
{
	
	VertexBufferObject* newVBO = GFXDevice::getInstance().newVBO();
	vector<U32> positionOffsets,normalsOffsets,texcoordOffsets;
	positionOffsets.push_back(0); //First VBO offsets
	normalsOffsets.push_back(0);
	texcoordOffsets.push_back(0);

	for(_subMeshIterator =  _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
	{
		newVBO->getPosition().insert(newVBO->getPosition().end(), (*_subMeshIterator)->getGeometryVBO()->getPosition().begin(), (*_subMeshIterator)->getGeometryVBO()->getPosition().end());
		positionOffsets.push_back((*_subMeshIterator)->getGeometryVBO()->getPosition().size()-1);

		newVBO->getNormal().insert(newVBO->getNormal().end(), (*_subMeshIterator)->getGeometryVBO()->getNormal().begin(), (*_subMeshIterator)->getGeometryVBO()->getNormal().end());
		normalsOffsets.push_back((*_subMeshIterator)->getGeometryVBO()->getNormal().size()-1);

		newVBO->getTexcoord().insert(newVBO->getTexcoord().end(), (*_subMeshIterator)->getGeometryVBO()->getTexcoord().begin(), (*_subMeshIterator)->getGeometryVBO()->getTexcoord().end());
		texcoordOffsets.push_back((*_subMeshIterator)->getGeometryVBO()->getTexcoord().size()-1);
	}
	return true;
}

bool Mesh::clean()
{
	if(_shouldDelete)
		return SceneManager::getInstance().getActiveScene()->removeGeometry(getName());
	else return false;
}