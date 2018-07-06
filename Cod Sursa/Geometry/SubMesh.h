#ifndef _SUB_MESH_H_
#define _SUB_MESH_H_
#include "Object3D.h"

/*
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

A SubMesh is a geometry wrapper used to build a mesh. Just like a mesh, it can be rendered locally or across
the server or disabled from rendering alltogheter. 

Objects created from this class have theyr position in relative space based on the parent mesh position.
(Same for scale,rotation and so on).

The SubMesh is composed of a VBO object that contains vertx,normal and textcoord data, a vector of materials,
and a name.
*/

#include "resource.h"
#include "Hardware/Video/VertexBufferObject.h"
#include "Hardware/Platform/PlatformDefines.h"

class SubMesh : public Object3D
{

public:
	SubMesh(const std::string& name) : Object3D(name){
									_geometry = GFXDevice::getInstance().newVBO();
									_geometryType = SUBMESH;}

	SubMesh(const SubMesh& old) : Object3D(old),
								 _render(old._render),_vboPositionOffset(old._vboPositionOffset){
		_geometry = GFXDevice::getInstance().newVBO();
		*_geometry = *(old._geometry);
		_indices.reserve(old._indices.size());
		for(U32 i = 0; i < old._indices.size(); i++)
			_indices.push_back(old._indices[i]);
		_material = old._material;
	}
	~SubMesh() {
		if(_geometry) {
			delete _geometry;
			_geometry = NULL;
		}
	}
	bool load(const std::string& name) { computeBoundingBox(); return true;}
	bool unload();

	inline VertexBufferObject* getGeometryVBO() {return _geometry;    } 
	inline std::vector<U32>&   getIndices()     {return _indices;     }
	inline Material&           getMaterial()	{return _material;    }   

private:
	void computeBoundingBox();

private:
	bool _visibleToNetwork, _render;
	VertexBufferObject*     _geometry;
	Material		        _material;
	std::vector<U32>        _indices;
	U32                     _vboPositionOffset;
};

#endif