#include "Mesh.h"
#include "Managers/ResourceManager.h"
#include "Rendering/Frustum.h"
#include "Managers/SceneManager.h"
#include "Utility/Headers/Quaternion.h"

Mesh::Mesh(const Mesh& old) : Object3D(old),
							  _computedLightShaders(old._computedLightShaders),
							  _visibleToNetwork(old._visibleToNetwork)
{
	vector<SubMesh* >::const_iterator it;
	vector<Shader* >::const_iterator it2;
	_subMeshes.reserve(old._subMeshes.size());
	_shaders.reserve(old._shaders.size());
	for(it = old._subMeshes.begin(); it != old._subMeshes.end(); ++it)
		_subMeshes.push_back(*it);
	for(it2 = old._shaders.begin(); it2 != old._shaders.end(); ++it2)
		_shaders.push_back(*it2);
}

void Mesh::onDraw()
{
	//onDraw must be called before any other rendering call (such as "isInView")
	//in order to properly compute the boundingbox
	setVisibility(SceneManager::getInstance().getActiveScene()->drawObjects());
	if(!getBoundingBox().isComputed())
		computeBoundingBox();
	_bb.Transform(_originalBB,_transform->getMatrix()); 
	drawBBox();
	if(!_computedLightShaders) 
		computeLightShaders();
}

bool Mesh::isVisible()
{
	if(!_render || !isInView() || _subMeshes.empty())
		return false;
	return true;

}
void Mesh::computeLightShaders()
{
	if(_shaders.empty())
	{
		vector<Light*>& lights = SceneManager::getInstance().getActiveScene()->getLights();
		for(U8 i = 0; i < lights.size(); i++)
			_shaders.push_back(ResourceManager::getInstance().LoadResource<Shader>("lighting"));
	}
	_computedLightShaders = true;
}

bool Mesh::isInView()
{
	if(!_render) return false;

	// Bellow code is still buggy. ToDo: FIX THIS!!!!!!!
	
	vec3 vEyeToChunk = getBoundingBox().getCenter() - Frustum::getInstance().getEyePos();
	if(vEyeToChunk.length() > SceneManager::getInstance().getTerrainManager()->getGeneralVisibility()) return false;
	// END BUGGY CODE :P

	vec3 center = getBoundingBox().getCenter();
	float radius = (getBoundingBox().getMax()-center).length();

	if(!getBoundingBox().ContainsPoint(Frustum::getInstance().getEyePos()))
	{
		switch(Frustum::getInstance().ContainsSphere(center, radius)) {
				case FRUSTUM_OUT: 	
					return false;
				
				case FRUSTUM_INTERSECT:	
					if(Frustum::getInstance().ContainsBoundingBox(getBoundingBox()) == FRUSTUM_OUT)
						return false;
				
			}
	}
	return true;
}



void Mesh::computeBoundingBox()
{
	_bb.setMin(vec3(100000.0f, 100000.0f, 100000.0f));
	_bb.setMax(vec3(-100000.0f, -100000.0f, -100000.0f));

	for(_subMeshIterator = _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
		for(int i=0; i<(int)(*_subMeshIterator)->getGeometryVBO()->getPosition().size(); i++)
		{
			vec3& position = (*_subMeshIterator)->getGeometryVBO()->getPosition()[i];
			_bb.Add(position);
		}
	_bb.isComputed() = true;
	_originalBB = _bb;
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

