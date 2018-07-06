#ifndef _SPHERE_3D_H_
#define _SPHERE_3D_H_
#include "Geometry/Object3D.h"

class Sphere3D : public Object3D
{
public:
	//Size is radius
	Sphere3D(F32 size,U8 resolution) : _size(size), _resolution(resolution)
			{_geometryType = SPHERE_3D;}
	
	bool load(const std::string &name) {_name = name; return true;}

	F32&			  getSize()       {return _size;}
	U8&			      getResolution() {return _resolution;}

	virtual void computeBoundingBox()
	{
		_bb.set(vec3(- _size,- _size,- _size), vec3( _size, _size, _size));
				_bb.isComputed() = true;
				_originalBB = _bb;
	}

protected:
	F32 _size;
	U8  _resolution;
	
};


#endif