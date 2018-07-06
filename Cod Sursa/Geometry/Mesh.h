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

class Mesh : public Object3D
{

public:
	Mesh() : Object3D() {}
	Mesh(vec3& position, vec3& scale, vec3& orientation)
		: Object3D(position,scale,orientation) {}

	void addSubMesh(SubMesh* subMesh){_subMeshes.push_back(subMesh);}
	inline vector<SubMesh* >& getSubMeshes() {return _subMeshes;}

	SubMesh*  getSubMesh(const string& name);
	void                computeBoundingBox();
	BoundingBox&        getBoundingBox() {return _bb;}
	Shader*             getShader()      {return _shader; }
	void                setShader(Shader* s) {_shader = s;}

	void Draw();
	void DrawBBox();
	bool IsInView();
	void setPosition(vec3 position);

protected:
	bool _visibleToNetwork,		 _render;
	vector<SubMesh* >			 _subMeshes;
	vector<SubMesh* >::iterator  _subMeshIterator;
	Shader*						 _shader;
	BoundingBox			         _bb;
};

#endif