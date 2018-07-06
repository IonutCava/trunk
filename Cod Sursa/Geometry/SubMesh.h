#ifndef _SUB_MESH_H_
#define _SUB_MESH_H_

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

class SubMesh
{

public:
	SubMesh(const string& name) : _name(name) {}
	VertexBufferObject* getGeometryVBO() {return _geometry;} 
	string& getName() {return _name;}
private:
	string _name;
	vec3 _position,_rotation,_scale;
	bool _visibleToNetwork, _render;
	VertexBufferObject* _geometry;
	//vector<Materials> _materials;
	
};

#endif