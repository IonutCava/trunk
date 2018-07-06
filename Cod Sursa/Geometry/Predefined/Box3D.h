#ifndef _BOX_3D_H_
#define _BOX_3D_H_
#include "Geometry/Object3D.h"

class Box3D : public Object3D
{
public:
	Box3D(F32 size) : _size(size) {_geometryType = BOX_3D;}
	bool load(const std::string &name) {_name = name; return true;}

	F32&         getSize()    {return _size;}
	virtual void computeBoundingBox()
	{
		_bb.set(vec3(-_size,-_size,-_size),vec3(_size,_size,_size));
		_bb.Multiply(0.5f);
		_bb.isComputed() = true;
		_originalBB = _bb;
	}
private:
	F32 _size;
};


#endif