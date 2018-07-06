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

#ifndef _QUAD_3D_H_
#define _QUAD_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"

class ShaderProgram;
class Quad3D : public Object3D {

public:
	Quad3D() :  Object3D(QUAD_3D,TRIANGLE_STRIP){

		vec3<F32> vertices[] = {vec3<F32>(-1.0f,  1.0f, 0.0f),   //TOP LEFT
						        vec3<F32>( 1.0f,  1.0f, 0.0f),   //TOP RIGHT
						        vec3<F32>(-1.0f, -1.0f, 0.0f),   //BOTTOM LEFT
						        vec3<F32>( 1.0f, -1.0f, 0.0f)};  //BOTTOM RIGHT

		vec3<F32> normals[] = {vec3<F32>(0, 0, 1), 
						       vec3<F32>(0, 0, 1), 
						       vec3<F32>(0, 0, 1),
						       vec3<F32>(0, 0, 1)};

		vec3<F32> tangents[] = {vec3<F32>(0.0f, 1.0f, 0.0f), 
						        vec3<F32>(0.0f, 1.0f, 0.0f), 
				                vec3<F32>(0.0f, 1.0f, 0.0f),
				                vec3<F32>(0.0f, 1.0f, 0.0f)};

		vec2<F32> texcoords[] = {vec2<F32>(0,1),
							     vec2<F32>(1,1),
							     vec2<F32>(0,0),
							     vec2<F32>(1,0)};

		_geometry->reservePositionCount(4);
		_geometry->getNormal().reserve(4);
		_geometry->getTangent().reserve(4);
		_geometry->getTexcoord().reserve(4);
		_geometry->getHWIndices().reserve(4);

		for(U8 i = 0;  i < 4; i++){
			_geometry->addPosition(vertices[i]);
			_geometry->getNormal().push_back(normals[i]);
			_geometry->getTangent().push_back(tangents[i]);
			_geometry->getTexcoord().push_back(texcoords[i]);
		}

	   //CCW draw order
	   _geometry->getHWIndices().push_back(2); //  v0----v1
	   _geometry->getHWIndices().push_back(0); //   |    |
	   _geometry->getHWIndices().push_back(1); //   |    |
	   _geometry->getHWIndices().push_back(1); //  v2----v3
       _geometry->getHWIndices().push_back(2);
       _geometry->getHWIndices().push_back(3);
  	   _geometry->setIndiceLimits(vec2<U16>(0,3));
	   _refreshVBO = true;
	   //computeTangents();
	}

	enum CornerLocation{
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT
	};

	vec3<F32> getCorner(CornerLocation corner){
		
		switch(corner){
			case TOP_LEFT: return _geometry->getPosition()[0];
			case TOP_RIGHT: return _geometry->getPosition()[1];
			case BOTTOM_LEFT: return _geometry->getPosition()[2];
			case BOTTOM_RIGHT: return _geometry->getPosition()[3];
			default: break;
		}
		return _geometry->getPosition()[0]; //default returns top left corner. Why? Don't care ... seems like a good idea. - Ionut
	}

	void setCorner(CornerLocation corner, const vec3<F32>& value){
		
		switch(corner){
	     	case TOP_LEFT:     _geometry->modifyPositionValue(0,value); break;
			case TOP_RIGHT:    _geometry->modifyPositionValue(1,value); break;
			case BOTTOM_LEFT:  _geometry->modifyPositionValue(2,value); break;
			case BOTTOM_RIGHT: _geometry->modifyPositionValue(3,value); break;
			default: break;
		}
		_refreshVBO = true;
		//computeTangents();
	}

	//rect.xy = Top Left; rect.zw = Bottom right
	//Remember to invert for 2D mode
	void setDimensions(const vec4<F32>& rect){
		_geometry->modifyPositionValue(0,vec3<F32>(rect.x, rect.w, 0));
		_geometry->modifyPositionValue(1,vec3<F32>(rect.z, rect.w, 0));
		_geometry->modifyPositionValue(2,vec3<F32>(rect.x, rect.y, 0));
		_geometry->modifyPositionValue(3,vec3<F32>(rect.z, rect.y, 0));
		_refreshVBO = true;
	}

	virtual bool computeBoundingBox(SceneGraphNode* const sgn) {
		if(sgn->getBoundingBox().isComputed()) return true;
		sgn->getBoundingBox().set(_geometry->getPosition()[2],_geometry->getPosition()[1]);
		return SceneNode::computeBoundingBox(sgn);
	}
};


#endif