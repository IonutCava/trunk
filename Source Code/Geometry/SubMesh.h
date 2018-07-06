/*“Copyright 2009-2011 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

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
	SubMesh(const std::string& name) : Object3D(name,SUBMESH),
		_geometry(GFXDevice::getInstance().newVBO()){}



	~SubMesh(){
		if(_geometry != NULL) {
			delete _geometry;
			_geometry = NULL;
		}
	}
	bool load(const std::string& name) {return true;}
	bool unload();
	bool computeBoundingBox(SceneGraphNode* node);
	inline VertexBufferObject* getGeometryVBO() {return _geometry;    } 
	inline std::vector<U32>&   getIndices()     {return _indices;     }
private:
	bool _visibleToNetwork, _render;
	VertexBufferObject*     _geometry;
	std::vector<U32>        _indices;
};

#endif