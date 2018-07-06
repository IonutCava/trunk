#include "SubMesh.h"
#include "Managers/ResourceManager.h"

void SubMesh::computeBoundingBox()
{
	_bb.setMin(vec3(100000.0f, 100000.0f, 100000.0f));
	_bb.setMax(vec3(-100000.0f, -100000.0f, -100000.0f));

	std::vector<vec3>&	tPosition	= _geometry->getPosition();

	for(U32 i=0; i < tPosition.size(); i++)
	{
		_bb.Add( tPosition[i] );
	}
	_originalBB = _bb;
	_bb.isComputed() = true;
}


bool SubMesh::unload()
{
	getIndices().clear(); 
	return true;
}