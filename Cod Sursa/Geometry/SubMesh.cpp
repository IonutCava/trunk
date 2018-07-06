#include "SubMesh.h"
#include "Managers/ResourceManager.h"

void SubMesh::computeBoundingBox()
{
	_bb.setMin(vec3(100000.0f, 100000.0f, 100000.0f));
	_bb.setMax(vec3(-100000.0f, -100000.0f, -100000.0f));

	std::vector<vec3>&	tPosition	= _geometry->getPosition();

	for(int i=0; i<(int)tPosition.size(); i++)
	{
		_bb.Add( tPosition[i] );
	}
}


bool SubMesh::unload()
{
	for(U8 i = 0; i < _material.textures.size(); i++)
		ResourceManager::getInstance().remove(_material.textures[i]->getName());
	if(_material.bumpMap)
		ResourceManager::getInstance().remove(_material.bumpMap->getName());
	getGeometryVBO()->Destroy();
	getIndices().clear(); 
	return true;

}