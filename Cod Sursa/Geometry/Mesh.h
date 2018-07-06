#ifndef _MESH_H_
#define _MESH_H_

/*
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

Mesh class. This class wraps all of the renderable geometry drawn by the engine. 
The only exceptions are: Terrain (including TerrainChunk) and Vegetation.

Meshes are composed of at least 1 submesh that contains vertex data, texture info and so on.
A mesh has a name, position, rotation, scale and a boolean value that enables or disables rendering 
across the network and one that disables rendering alltogheter;

Note: all transformations applied to the mesh affect every submesh that compose the mesh. 
*/

#include "resource.h"
#include "SubMesh.h"
#include <unordered_map>

class Mesh
{
public:
	Mesh(const string& name) : _name(name)
	{}
	void addSubMesh(SubMesh* subMesh){_subMeshes.insert(pair<string,SubMesh*>(subMesh->getName(),subMesh));}

private:
	string _name;
	vec3 _position,_rotation,_scale;
	bool _visibleToNetwork, _render;
	tr1::unordered_map<string, SubMesh*> _subMeshes;
};

#endif