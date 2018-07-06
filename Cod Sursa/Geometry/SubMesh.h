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
#include "Utility/Headers/DataTypes.h"

class SubMesh : public Object3D
{

public:
	SubMesh(const string& name) : Object3D(name),
								 _geometry(GFXDevice::getInstance().newVBO()){_geometryType = SUBMESH;}

	bool load(const string& name) { computeBoundingBox(); return true;}
	bool unload();

	inline VertexBufferObject* getGeometryVBO() {return _geometry;    } 
	inline vector<U32>&        getIndices()     {return _indices;     }
	inline Material&           getMaterial()	{return _material;    }   
	inline string&             getName()		{return _name;        }

private:
	void computeBoundingBox();
	bool _visibleToNetwork, _render;
	VertexBufferObject* _geometry;
	Material		    _material;
	vector<U32>         _indices;
	U32                 _vboPositionOffset;
};

#endif