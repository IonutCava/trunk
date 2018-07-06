#include "SubMesh.h"
#include "Managers/ResourceManager.h"

void SubMesh::computeBoundingBox()
{
	_boundingBox.min = vec3(100000.0f, 100000.0f, 100000.0f);
	_boundingBox.max = vec3(-100000.0f, -100000.0f, -100000.0f);

	std::vector<vec3>&	tPosition	= _geometry->getPosition();

	for(int i=0; i<(int)tPosition.size(); i++)
	{
		_boundingBox.Add( tPosition[i] );
	}
}


bool SubMesh::unload()
{
	ResourceManager::getInstance().remove(_material.texture->getName());
	getGeometryVBO()->Destroy();
	getIndices().clear(); 
	return true;

}