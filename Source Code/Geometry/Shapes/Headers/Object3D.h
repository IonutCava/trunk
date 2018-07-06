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
	enum PrimitiveType {
		OBJECT_3D = 0,
		SPHERE_3D,
		BOX_3D,
		QUAD_3D,
		TEXT_3D,
		MESH,
		SUBMESH,
		GENERIC,
		OBJECT_3D_FLYWEIGHT
	};

	enum PrimitiveFlag {
		PRIMITIVE_FLAG_NONE = 0,
		PRIMITIVE_FLAG_SKINNED,
		PRIMITIVE_FLAG_PLACEHOLDER

	};
	enum UsageContext {
		OBJECT_DYNAMIC = 0,
		OBJECT_STATIC
	};

	Object3D(PrimitiveType type = OBJECT_3D,PrimitiveFlag flag = PRIMITIVE_FLAG_NONE);
	Object3D(const std::string& name, PrimitiveType type = OBJECT_3D, PrimitiveFlag flag = PRIMITIVE_FLAG_NONE);

	virtual ~Object3D(){
		SAFE_DELETE(_geometry);
	};

	        VertexBufferObject* const getGeometryVBO(); ///<Please use IBO's ...
	inline  PrimitiveType             getType()         const {return _geometryType;}
	inline  PrimitiveFlag             getFlag()         const {return _geometryFlag;}
	inline  UsageContext              getUsageContext() const {return _usageContext;}
	inline  void                      setUsageContext(UsageContext newContext) {_usageContext = newContext;}

	/// Called from SceneGraph "sceneUpdate"
	virtual void  sceneUpdate(D32 sceneTime) {}           ///<To avoid a lot of typing
	virtual void  postLoad(SceneGraphNode* const sgn) {} ///<To avoid a lot of typing
	virtual	void  render(SceneGraphNode* const sgn);
	virtual void  onDraw();


protected:
	virtual void computeNormals() {};
	virtual void computeTangents();

protected:
	bool		          _update;
	bool                  _refreshVBO;
	PrimitiveType         _geometryType;
	PrimitiveFlag         _geometryFlag;
	UsageContext          _usageContext;
	VertexBufferObject*   _geometry;
};

#endif