#ifndef _QUATERNION_H_
#define _QUATERNION_H_
/*
http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation
Quaternion class based on code from " OpenGL:Tutorials:Using Quaternions to represent rotation "
*/
#include "resource.h"
#define TOLERANCE 0.00001f

class Quaternion
{
public:
	Quaternion(F32 x, F32 y, F32 z, F32 w) : _x(x), _y(y), _z(z), _w(w),_dirty(true),_mat(NULL) {}
	Quaternion() : _x(0), _y(0), _z(0), _w(0),_dirty(true),_mat(NULL) {}

	void       normalize();
	Quaternion getConjugate() const;
	Quaternion operator* (const Quaternion &rq) const;
	vec3       operator* (const vec3 &vec) const;

	void	   FromAxis(const vec3& v, float angle);
	void       FromEuler(float pitch, float yaw, float roll);
	void       FromEuler(const vec3& v) {FromEuler(v.x,v.y,v.z);}
	mat4&      getMatrix();
	void       getAxisAngle(vec3 *axis, float *angle,bool inDegrees);

private:
	F32 _x,_y,_z,_w;
	mat4 *_mat;
	bool _dirty;
};
#endif