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

#include "Graphs/Headers/SceneGraphNode.h"

class BoundingBox;
class RenderInstance;
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
		OBJECT_3D_FLYWEIGHT,
        OBJECT_3D_PLACEHOLDER
	};

	enum ObjectFlag {
		OBJECT_FLAG_NONE = 0,
		OBJECT_FLAG_SKINNED,
		OBJECT_FLAG_PLACEHOLDER
	};

	Object3D(const ObjectType& type = OBJECT_3D_PLACEHOLDER, const PrimitiveType& vboType = TRIANGLES,const ObjectFlag& flag = OBJECT_FLAG_NONE);
	Object3D(const std::string& name,const ObjectType& type = OBJECT_3D_PLACEHOLDER,const PrimitiveType& vboType = TRIANGLES,const ObjectFlag& flag = OBJECT_FLAG_NONE);

	virtual ~Object3D();

	inline  VertexBufferObject* const getGeometryVBO()  const {assert(_geometry != NULL); return _geometry;}
	inline  ObjectType                getType()         const {return _geometryType;}
	inline  ObjectFlag                getFlag()         const {return _geometryFlag;}
	inline  RenderInstance*   const   renderInstance()  const {return _renderInstance;}
	/// Called from SceneGraph "sceneUpdate"
	virtual void  sceneUpdate(const U32 sceneTime,SceneGraphNode* const sgn) {SceneNode::sceneUpdate(sceneTime,sgn);}     //<To avoid a lot of typing
	virtual void  postLoad(SceneGraphNode* const sgn) {} ///<To avoid a lot of typing
	virtual	void  render(SceneGraphNode* const sgn);
	virtual void  onDraw(const RenderStage& currentStage);
    //virtual void  optimizeForDepth(bool state = true,bool force = false) {if(_geometry) _geometry->optimizeForDepth(state,force);}

protected:
	virtual void computeNormals();
	virtual void computeTangents();

protected:
	bool		          _update;
	ObjectType            _geometryType;
	ObjectFlag            _geometryFlag;
	VertexBufferObject*   _geometry;
	///The actual render instance needed by the rendering API
	RenderInstance*       _renderInstance;
};

#endif