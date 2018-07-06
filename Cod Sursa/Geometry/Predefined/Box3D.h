#ifndef _BOX_3D_H_
#define _BOX_3D_H_
#include "Geometry/Object3D.h"

class Box3D : public Object3D
{
public:
	Box3D(F32 size) : _size(size) {};
	~Box3D(){};
	bool load(const std::string &name) {return true;}
	bool unload() {return true;}
	void draw();
private:
	F32 _size;
};


#endif