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

#include "Geometry/Shapes/Headers/Object3D.h"

class Sphere3D : public Object3D
{
public:
	//Size is radius
	Sphere3D(F32 size, F32 resolution) : Object3D(SPHERE_3D),
										_size(size),
										_resolution(resolution){
											_dirty = true;
										}
	
	bool load(const std::string &name) {_name = name; return true;}

	inline F32	  getSize()    {return _size;}
	inline F32    getResolution() {return _resolution;}
	inline void   setSize(F32 size) {_size = size; _dirty = true; _refreshVBO = true;}
	inline void   setResolution(F32 resolution) {_resolution = resolution; _dirty = true; _refreshVBO = true;}

	virtual bool computeBoundingBox(SceneGraphNode* const node){
		if(node->getBoundingBox().isComputed()) return true;
		node->getBoundingBox().set(vec3(- _size,- _size,- _size), vec3( _size, _size, _size));
		return SceneNode::computeBoundingBox(node);
	}

	void onDraw(){
		if(_dirty){
			createSphere();
			_dirty = false;
		}
		Object3D::onDraw();
	}

private:

	void createSphere(){
		//Code from: http://stackoverflow.com/a/5989676 
		//By datenwolf (http://stackoverflow.com/users/524368/datenwolf)

		I32 rings = _resolution;
		I32 sectors = _resolution;
		F32 radius = _size;
		
	    F32 const R = 1./(F32)(rings-1);
        F32 const S = 1./(F32)(sectors-1);
        I32 r, s;

        _geometry->getPosition().reserve(rings * sectors);
        _geometry->getNormal().reserve(rings * sectors);
        _geometry->getTexcoord().reserve(rings * sectors);
        _indices.resize(rings * sectors * 4);

 		for(r = 0; r < rings; r++){
			for(s = 0; s < sectors; s++) {
                F32 const y = sin( -M_PIDIV2 + M_PI * r * R );
                F32 const x = cos(2*M_PI * s * S) * sin( M_PI * r * R );
                F32 const z = sin(2*M_PI * s * S) * sin( M_PI * r * R );

				_geometry->getTexcoord().push_back(vec2(s*S,r*R));
                _geometry->getPosition().push_back(vec3(x * radius,y * radius, z * radius));
				_geometry->getNormal().push_back(vec3(x,y,z));
			}
		}


		for(r = 0; r < rings; r++){
			for(s = 0; s < sectors; s++) {
				_indices.push_back(r * sectors + s);
                _indices.push_back(r * sectors + (s+1));
                _indices.push_back((r+1) * sectors + (s+1));
                _indices.push_back((r+1) * sectors + s);
	        }
		}
		_refreshVBO = true;
	}
	

protected:
	F32 _size, _resolution;
	U32 _vertexCount;
	bool _dirty;
	
};


#endif