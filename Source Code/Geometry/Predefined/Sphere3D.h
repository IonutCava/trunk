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

#ifndef _SPHERE_3D_H_
#define _SPHERE_3D_H_
#include "Geometry/Object3D.h"

class Sphere3D : public Object3D
{
public:
	//Size is radius
	Sphere3D(F32 size,U8 resolution) : Object3D(SPHERE_3D),
									   _size(size),
									   _resolution(resolution)
									  {}
	
	bool load(const std::string &name) {_name = name; return true;}

	F32&			  getSize()       {return _size;}
	U8&			      getResolution() {return _resolution;}

	virtual bool computeBoundingBox(SceneGraphNode* node){
		node->getBoundingBox().set(vec3(- _size,- _size,- _size), vec3( _size, _size, _size));
		return SceneNode::computeBoundingBox(node);
	}

protected:
	F32 _size;
	U8  _resolution;
	
};


#endif