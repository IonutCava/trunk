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

#ifndef _BOX_3D_H_
#define _BOX_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"

class Box3D : public Object3D
{
public:
	Box3D(F32 size) :  Object3D(BOX_3D), _size(size){
	
		vec3<F32> vertices[] = {vec3<F32>(-1.0f, -1.0f, 1.0f), 
							    vec3<F32>(1.0f, -1.0f, 1.0f), 
							    vec3<F32>(1.0f, 1.0f, 1.0f), 
							    vec3<F32>(-1.0f, 1.0f, 1.0f), 
							    vec3<F32>(-1.0f, -1.0f, -1.0f), 
							    vec3<F32>(1.0f, -1.0f, -1.0f), 
							    vec3<F32>(1.0f, 1.0f, -1.0f), 
							    vec3<F32>(-1.0f, 1.0f, -1.0f)};

		vec3<F32> normals[] = {vec3<F32>(-1.0f, -1.0f, 1.0f), 
							   vec3<F32>(1.0f, -1.0f, 1.0f), 
							   vec3<F32>(1.0f, 1.0f, 1.0f), 
							   vec3<F32>(-1.0f, 1.0f, 1.0f), 
							   vec3<F32>(-1.0f, -1.0f, -1.0f), 
							   vec3<F32>(1.0f, -1.0f, -1.0f), 
							   vec3<F32>(1.0f, 1.0f, -1.0f), 
							   vec3<F32>(-1.0f, 1.0f, -1.0f)};

		U16 indices[] = {0, 1, 2, 2, 3, 0, 
						 3, 2, 6, 6, 7, 3, 
						 7, 6, 5, 5, 4, 7, 
						 4, 0, 3, 3, 7, 4, 
						 0, 1, 5, 5, 4, 0,
						 1, 5, 6, 6, 2, 1 };

		_geometry->getPosition().reserve(8);
		_geometry->getNormal().reserve(8);
		_geometry->getHWIndices().reserve(36);
		_indiceLimits = vec2<U16>(0,7);
		F32 halfExtent = size*0.5f;

		for(U8 i = 0; i < 8; i++){
			_geometry->getPosition().push_back(vertices[i] * halfExtent);
			_geometry->getNormal().push_back(normals[i]);
		}

		for(U8 i = 0; i < 36; i++){
			_geometry->getHWIndices().push_back(indices[i]);
		}

		_refreshVBO = true;
	}

	inline F32 getSize()    {return _size;}

	inline void  setSize(F32 size) {
		///Since the initial box is half of the full extent already (in the constructor)
		///Each vertex is already multiplied by 0.5 so, just multiply by the new size
		///IMPORTANT!! -be aware of this - Ionut
		F32 halfExtent = size;

		for(U8 i = 0; i < 8; i++){
			_geometry->getPosition()[i] *= halfExtent;
		}

		_refreshVBO = true;
		_size = size;
	}


	virtual bool computeBoundingBox(SceneGraphNode* const sgn){
		if(sgn->getBoundingBox().isComputed()) return true;
		sgn->getBoundingBox().set(vec3<F32>(-_size,-_size,-_size),vec3<F32>(_size,_size,_size));
		sgn->getBoundingBox().Multiply(0.5f);
		return SceneNode::computeBoundingBox(sgn);
	}
private:
	F32 _size;
};


#endif