#ifndef _OBJECT_3D_H
#define _OBJECT_3D_H
#include "resource.h"

class Object3D
{
public:
	~Object3D(){}
	Object3D() : _position(0,0,0),
		         _scale(1,1,1),
				 _orientation(0,0,-1),
				 _name("default"){};
	
	Object3D(vec3& position) : _position(position),
						      _scale(1,1,1),
							  _orientation(0,0,-1),
							  _name("default"){};
	
	Object3D(vec3& position, vec3& scale) : _position(position),
											_scale(scale),
											_orientation(0,0,-1),
										    _name("default") {};

	Object3D(vec3& position, vec3& scale, vec3& orientation) : _position(position),
															   _scale(scale),
															   _orientation(orientation),
															   _name("default") {}

	vec3&						getPosition(){return _position;}
	vec3&						getOrientation(){return _orientation;}
	vec3&						getScale(){return _scale;}
	string&						getName(){return _name;}

	inline void translate(F32 x, F32 y, F32 z){_position+vec3(x,y,z);}
	inline void scale(F32 x, F32 y, F32 z){_position+vec3(x,y,z);}
	inline void orient(F32 x, F32 y, F32 z){_orientation = vec3(x,y,z);}

protected:
	vec3 _position, _orientation, _scale;

private:
	string _name;
};

#endif