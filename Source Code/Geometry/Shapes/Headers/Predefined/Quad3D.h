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

#include "Geometry/Shapes/Headers/Object3D.h"

class Shader;
class Quad3D : public Object3D
{
public:
	Quad3D() :  Object3D(QUAD_3D){
			   vec3 normal(0,1,0);
			   vec3 tangent(1,0,0);
			   //TOP LEFT
			   _geometry->getPosition().push_back(vec3(-1,1,0));
			   _geometry->getNormal().push_back(normal);
			   _geometry->getTangent().push_back(tangent);
			   _geometry->getTexcoord().push_back(vec2(0,0));
			   //TOP RIGHT
			   _geometry->getPosition().push_back(vec3(1,1,0));
			   _geometry->getNormal().push_back(normal);
			   _geometry->getTangent().push_back(tangent);
			   _geometry->getTexcoord().push_back(vec2(1,0));
				//BOTTOM LEFT
			   _geometry->getPosition().push_back(vec3(-1,-1,0));
			   _geometry->getNormal().push_back(normal);
			   _geometry->getTangent().push_back(tangent);
			   _geometry->getTexcoord().push_back(vec2(0,1));
			   //BOTTOM RIGHT
			   _geometry->getPosition().push_back(vec3(1,-1,0));
			   _geometry->getNormal().push_back(normal);
			   _geometry->getTangent().push_back(tangent);
			   _geometry->getTexcoord().push_back(vec2(1,1));

			   _indices.push_back(0);
			   _indices.push_back(1);
			   _indices.push_back(3);
			   _indices.push_back(2);
			   _refreshVBO = true;
			   //computeTangents();
	}

	bool load(const std::string &name) {
		
		return true;
	}

	enum CornerLocation{
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT
	};

	vec3 getCorner(CornerLocation corner){
		//In 2D mode, Quad's are flipped!!!!
		switch(corner){
			case TOP_LEFT: return _geometry->getPosition()[0];
			case TOP_RIGHT: return _geometry->getPosition()[1];
			case BOTTOM_LEFT: return _geometry->getPosition()[2];
			case BOTTOM_RIGHT: return _geometry->getPosition()[3];
			default: break;
		}
		return _geometry->getPosition()[0]; //default returns top left corner. Why? Don't care ... seems like a good idea. - Ionut
	}

	void setCorner(CornerLocation corner, const vec3& value){
		//In 2D mode, Quad's are flipped!!!!
		switch(corner){
	     	case TOP_LEFT:     _geometry->getPosition()[0] = value; break;
			case TOP_RIGHT:    _geometry->getPosition()[1] = value; break;
			case BOTTOM_LEFT:  _geometry->getPosition()[2] = value; break;
			case BOTTOM_RIGHT: _geometry->getPosition()[3] = value; break;
			default: break;
		}
		_refreshVBO = true;
		//computeTangents();
	}

	virtual bool computeBoundingBox(SceneGraphNode* const node) {
		if(node->getBoundingBox().isComputed()) return true;
		node->getBoundingBox().set(_geometry->getPosition()[2],_geometry->getPosition()[1]);
		return SceneNode::computeBoundingBox(node);
	}
};


#endif