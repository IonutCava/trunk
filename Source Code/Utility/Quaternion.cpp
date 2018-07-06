#include "Headers/Quaternion.h"


// normalising a quaternion works similar to a vector. This method will not do anything
// if the quaternion is close enough to being unit-length. define TOLERANCE as something
// small like 0.00001f to get accurate results
void Quaternion::normalize(){
	_dirty = true;
	// Don't normalize if we don't have to
	F32 mag2 = _w * _w + _x * _x + _y * _y + _z * _z;
	if (  mag2!=0.f && (fabs(mag2 - 1.0f) > TOLERANCE)) {
		D32 mag = Util::square_root(mag2);
		_w /= mag;
		_x /= mag;
		_y /= mag;
		_z /= mag;
	}
}

// We need to get the inverse of a quaternion to properly apply a quaternion-rotation to a vector
// The conjugate of a quaternion is the same as the inverse, as long as the quaternion is unit-length
Quaternion Quaternion::getConjugate() const{
	return Quaternion(-_x, -_y, -_z, _w);
}

// Multiplying q1 with q2 applies the rotation q2 to q1
Quaternion Quaternion::operator* (const Quaternion &rq) const{
	// the constructor takes its arguments as (x, y, z, w)
	return Quaternion(_w * rq._x + _x * rq._w + _y * rq._z - _z * rq._y,
	                  _w * rq._y + _y * rq._w + _z * rq._x - _x * rq._z,
	                  _w * rq._z + _z * rq._w + _x * rq._y - _y * rq._x,
	                  _w * rq._w - _x * rq._x - _y * rq._y - _z * rq._z);
}

// Multiplying a quaternion q with a vector v applies the q-rotation to v
vec3 Quaternion::operator* (const vec3 &vec) const{
	vec3 vn(vec);
	vn.normalize();
 
	Quaternion vecQuat, resQuat;
	vecQuat._x = vn.x;
	vecQuat._y = vn.y;
	vecQuat._z = vn.z;
	vecQuat._w = 0.0f;
 
	resQuat = vecQuat * getConjugate();
	resQuat = *this * resQuat;
 
	return (vec3(resQuat._x, resQuat._y, resQuat._z));
}

// Convert from Axis Angle
void Quaternion::FromAxis(const vec3 &v, F32 angle){
	_dirty = true;
	F32 sinAngle;
	angle = RADIANS(angle);
	angle *= 0.5f;
	vec3 vn(v);
	vn.normalize();
 
	sinAngle = sin(angle);
 
	_x = (vn.x * sinAngle);
	_y = (vn.y * sinAngle);
	_z = (vn.z * sinAngle);
	_w = cos(angle);

}

// Convert from Euler Angles
void Quaternion::FromEuler(F32 pitch, F32 yaw, F32 roll){
	_dirty = true;
	// Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll
	// and multiply those together.
	// the calculation below does the same, just shorter
 
	F32 p = pitch * M_PIDIV180 / 2.0;
	F32 y = yaw * M_PIDIV180 / 2.0;
	F32 r = roll * M_PIDIV180 / 2.0;
 
	F32 sinp = sin(p);
	F32 siny = sin(y);
	F32 sinr = sin(r);
	F32 cosp = cos(p);
	F32 cosy = cos(y);
	F32 cosr = cos(r);
 
	this->_x = sinr * cosp * cosy - cosr * sinp * siny;
	this->_y = cosr * sinp * cosy + sinr * cosp * siny;
	this->_z = cosr * cosp * siny - sinr * sinp * cosy;
	this->_w = cosr * cosp * cosy + sinr * sinp * siny;
	normalize();
}

// Convert to Matrix
mat4& Quaternion::getMatrix(){
	if(_dirty) {
		delete _mat;
		F32 x2 = _x * _x;
		F32 y2 = _y * _y;
		F32 z2 = _z * _z;
		F32 xy = _x * _y;
		F32 xz = _x * _z;
		F32 yz = _y * _z;
		F32 wx = _w * _x;
		F32 wy = _w * _y;
		F32 wz = _w * _z;

		_mat = New mat4( 1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
					2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx), 0.0f,
					2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2), 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
		_dirty = false;
	}
	return *_mat;
}

// Convert to Axis/Angles
void Quaternion::getAxisAngle(vec3 *axis, F32 *angle,bool inDegrees){
	F32 scale = Util::square_root(_x * _x + _y * _y + _z * _z);
	axis->x = _x / scale;
	axis->y = _y / scale;
	axis->z = _z / scale;
	if(inDegrees)
		*angle = DEGREES(acos(_w) * 2.0f);
	else
		*angle = acos(_w) * 2.0f;
}

bool Quaternion::compare(Quaternion& q){
	vec4 thisQ(_x,_y,_z,_w);
	vec4 otherQ(q._x,q._y,q._z,q._w);

	return thisQ.compare(otherQ);
}