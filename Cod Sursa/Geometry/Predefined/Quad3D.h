#ifndef _QUAD_3D_H_
#define _QUAD_3D_H_
#include "Geometry/Object3D.h"

class Quad3D : public Object3D
{
public:
	Quad3D(vec3& topLeft, vec3& topRight, vec3& bottomLeft, vec3& bottomRight) : _tl(topLeft),
																				 _tr(topRight),
																				 _bl(bottomLeft),
																				 _br(bottomRight){};
	~Quad3D(){};
	bool load(const std::string &name) {return true;}
	bool unload() {return true;}
	void draw();
private:
	vec3 _tl,_tr,_bl,_br;
};


#endif