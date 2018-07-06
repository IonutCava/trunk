#ifndef _SPHERE_3D_H_
#define _SPHERE_3D_H_
#include "Geometry/Object3D.h"

class Sphere3D : public Object3D
{
public:
	Sphere3D(F32 size,U32 resolution) : _size(size), _resolution(resolution) {};
	~Sphere3D(){};
	bool load(const std::string &name) {return true;}
	bool unload() {return true;}
	F32& getSize() {return _size;}
	U32& getResolution() {return _resolution;}
private:
	F32 _size;
	U32 _resolution;
};


#endif