/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#ifndef _OBJECT_3D_H_
#define _OBJECT_3D_H_

#include "core.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"

class BoundingBox;
class VertexBufferObject;
class Object3D : public SceneNode {
public:
	enum ObjectType {
		SPHERE_3D = 0,
		BOX_3D,
		QUAD_3D,
		TEXT_3D,
		MESH,
		SUBMESH,
		GENERIC,
		OBJECT_3D_FLYWEIGHT,
        OBJECT_3D_PLACEHOLDER
	};

	enum ObjectFlag {
		OBJECT_FLAG_NONE = 0,
		OBJECT_FLAG_SKINNED,
		OBJECT_FLAG_PLACEHOLDER

	};

	Object3D(ObjectType type = OBJECT_3D_PLACEHOLDER, PrimitiveType vboType = TRIANGLES, ObjectFlag flag = OBJECT_FLAG_NONE);
	Object3D(const std::string& name, ObjectType type = OBJECT_3D_PLACEHOLDER, PrimitiveType vboType = TRIANGLES, ObjectFlag flag = OBJECT_FLAG_NONE);

	virtual ~Object3D(){
		SAFE_DELETE(_geometry);
	};

	        VertexBufferObject* const getGeometryVBO(); ///<Please use IBO's ...
	inline  ObjectType                getType()         const {return _geometryType;}
	inline  ObjectFlag                getFlag()         const {return _geometryFlag;}

	/// Called from SceneGraph "sceneUpdate"
	virtual void  sceneUpdate(U32 sceneTime) {}           ///<To avoid a lot of typing
	virtual void  postLoad(SceneGraphNode* const sgn) {} ///<To avoid a lot of typing
	virtual	void  render(SceneGraphNode* const sgn);
	virtual void  onDraw();
    //virtual void  optimizeForDepth(bool state = true,bool force = false) {if(_geometry) _geometry->optimizeForDepth(state,force);}

protected:
	virtual void computeNormals() {};
	virtual void computeTangents();

protected:
	bool		          _update;
	bool                  _refreshVBO;
	ObjectType            _geometryType;
	ObjectFlag            _geometryFlag;
	VertexBufferObject*   _geometry;
};

#endif