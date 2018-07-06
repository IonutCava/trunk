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

#ifndef _SPHERE_3D_H_
#define _SPHERE_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

class Sphere3D : public Object3D {
public:
	///Change resolution to affect the spacing between vertices
	Sphere3D(F32 radius, F32 resolution) : Object3D(SPHERE_3D,TRIANGLE_STRIP),
										  _radius(radius),
										  _resolution(resolution)
	{
		_dirty = true;
		_vertexCount = resolution * resolution;
		_geometry->reservePositionCount(_vertexCount);
        _geometry->reserveNormalCount(_vertexCount);
        _geometry->getTexcoord().reserve(_vertexCount);
        _geometry->reserveIndexCount(_vertexCount);
		if(_vertexCount+1 > std::numeric_limits<U16>::max()){
			_geometry->useLargeIndices(true);
		}
	}

	inline F32	  getRadius()     {return _radius;}
	inline F32    getResolution() {return _resolution;}
	inline void   setRadius(F32 radius) {_radius = radius; _dirty = true; _geometry->queueRefresh();}
	inline void   setResolution(F32 resolution) {_resolution = resolution; _dirty = true; _geometry->queueRefresh();}

	virtual bool computeBoundingBox(SceneGraphNode* const sgn){
		if(sgn->getBoundingBox().isComputed()) return true;
		sgn->getBoundingBox().set(vec3<F32>(- _radius,- _radius,- _radius), vec3<F32>( _radius, _radius, _radius));
		return SceneNode::computeBoundingBox(sgn);
	}

	void onDraw(const RenderStage& currentStage){
		if(_dirty){
			createSphere(_resolution,_resolution);
			_dirty = false;
		}
		Object3D::onDraw(currentStage);
	}

private:

	//SuperBible stuff
	void createSphere(U32 slices, U32 stacks) {
		_geometry->Reset();
		F32 drho = M_PI / (F32) stacks;
		F32 dtheta = 2.0f * M_PI / (F32) slices;
		F32 ds = 1.0f / (F32) slices;
		F32 dt = 1.0f / (F32) stacks;
		F32 t = 1.0f;
		F32 s = 0.0f;
		F32 i, j;     // Looping variables
		U32 index = 0; ///for the index buffer

		for (i = 0; i < stacks; i++) {
			F32 rho  = i * drho;
			F32 srho = sin(rho);
		    F32 crho = cos(rho);
		    F32 srhodrho = sin(rho + drho);
		    F32 crhodrho = cos(rho + drho);

			// Many sources of OpenGL sphere drawing code uses a triangle fan
			// for the caps of the sphere. This however introduces texturing
			// artifacts at the poles on some OpenGL implementations
	        s = 0.0f;
			for ( j = 0; j <= slices; j++) {
				F32 theta  = (j == slices) ? 0.0f : j * dtheta;
				F32 stheta = -sin(theta);
			    F32 ctheta = cos(theta);

				F32 x = stheta * srho;
				F32 y = ctheta * srho;
				F32 z = crho;

				_geometry->addPosition(vec3<F32>(x * _radius, y * _radius, z * _radius));
				_geometry->getTexcoord().push_back(vec2<F32>(s,t));
				_geometry->addNormal(vec3<F32>(x,y,z));
				_geometry->addIndex(index++);

				x = stheta * srhodrho;
				y = ctheta * srhodrho;
				z = crhodrho;
				s += ds;

				_geometry->addPosition(vec3<F32>(x * _radius, y * _radius, z * _radius));
				_geometry->getTexcoord().push_back(vec2<F32>(s,t - dt));
				_geometry->addNormal(vec3<F32>(x,y,z));
				_geometry->addIndex(index++);
			}
	        t -= dt;
		}

		vec2<U32> indiceLimits;
		for(U32 i = 0 ; i < _geometry->getIndexCount(); i++){
			if(indiceLimits.x > _geometry->getIndex(i))
				indiceLimits.x = _geometry->getIndex(i);
			if(indiceLimits.y < _geometry->getIndex(i))
				indiceLimits.y = _geometry->getIndex(i);
		}

		_geometry->setIndiceLimits(indiceLimits);
		_geometry->queueRefresh();
	}

protected:
	F32 _radius, _resolution;
	U32 _vertexCount;
	bool _dirty;
};

#endif