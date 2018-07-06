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

#ifndef _QUAD_3D_H_
#define _QUAD_3D_H_
#include "Geometry/Object3D.h"

class Shader;
class Quad3D : public Object3D
{
public:
	Quad3D(const vec3& topLeft,const  vec3& topRight,
		   const  vec3& bottomLeft,const  vec3& bottomRight) :  Object3D(QUAD_3D),
															   _tl(topLeft),
		    												   _tr(topRight),
															   _bl(bottomLeft),
															   _br(bottomRight)
															   {}
	Quad3D() :  Object3D(QUAD_3D),
			   _tl(vec3(1,1,0)),
			   _tr(vec3(-1,0,0)),
			   _bl(vec3(0,-1,0)),
			   _br(vec3(-1,-1,0))
			   {}

	bool load(const std::string &name) {_name = name; return true;}

	enum CornerLocation{
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT
	} ;

	vec3& getCorner(CornerLocation corner){
		switch(corner){
			case TOP_LEFT: return _tl;
			case TOP_RIGHT: return _tr;
			case BOTTOM_LEFT: return _bl;
			case BOTTOM_RIGHT: return _br;
			default: break;
		}
		return _tl; //default returns top left corner. Why? Don't care ... seems like a good idea. - Ionut
	}
	
	virtual bool computeBoundingBox() {
		_boundingBox.setMin(_bl);
		_boundingBox.setMax(_tr);
		_boundingBox.isComputed() = true;
		setOriginalBoundingBox(_boundingBox);
		return true;
	}

public: 



private:
	vec3 _tl,_tr,_bl,_br;
};


#endif