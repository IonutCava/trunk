/* Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _CORE_MATH_MATH_VECTORS_H_
#define _CORE_MATH_MATH_VECTORS_H_

/*
*  useful vector functions
*/
/// general vec2 cross function
template<class T>
inline vec2<T> Cross(const vec2<T> &v1, const vec2<T> &v2) {
	return v1.x * v2.y - v1.y * v2.x;
}

/// multiply a vector by a value
template<class T>
inline vec2<T> operator*(T fl, const vec2<T>& v) { 
	return vec2<T>(v.x*fl, v.y*fl);
}

/// general vec2 dot product
template<class T>
inline T Dot(const vec2<T>& a, const vec2<T>& b) {
	return(a.x*b.x+a.y*b.y); 
}

/// multiply a vector by a value
template<class T>
inline vec3<T> operator*(T fl, const vec3<T>& v) { 
	return vec3<T>(v.x*fl, v.y*fl, v.z*fl);
}

/// general vec3 dot product
template<class T>
inline T Dot(const vec3<T>& a, const vec3<T>& b) { 
	return(a.x*b.x+a.y*b.y+a.z*b.z);
}

/// general vec3 cross function
template<class T>
inline vec3<T> Cross(const vec3<T> &v1, const vec3<T> &v2) {
	return vec3<T>(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

/// multiply a vector by a value
template<class T>
inline vec4<T> operator*(T fl, const vec4<T>& v) { 
	return vec4<T>(v.x*fl, v.y*fl, v.z*fl,  v.w*fl);
}

/*
*  vec2 inline definitions
*/

/// convert the vector to unit length
template<class T>
inline T vec2<T>::normalize() {
	T l = this->length();

	if(l < EPSILON) 
		return 0;

	T inv = 1.0f / l;
	this->x *= inv;
	this->y *= inv;
	return l;
}

/// compare 2 vectors using the given tolerance
template<class T>
inline bool vec2<T>::compare(const vec2 &_v,F32 epsi=EPSILON) const { 
	return (FLOAT_COMPARE_TOLERANCE(this->x,_v.x,epsi) && 
			FLOAT_COMPARE_TOLERANCE(this->y,_v.y,epsi)); 
}

/// return the coordinates of the closest point from *this to the line determined by points vA and vB
template<class T>
inline vec2<T> vec2<T>::closestPointOnLine(const vec2 &vA, const vec2 &vB) const { 
	return (((vB-vA) * this->projectionOnLine(vA, vB)) + vA);
}

/// return the coordinates of the closest point from *this to the segment determined by points vA and vB
template<class T>
inline vec2<T> vec2<T>::closestPointOnSegment(const vec2 &vA, const vec2 &vB) const {
	T factor = this->projectionOnLine(vA, vB);

	if (factor <= 0) 
		return vA;

	if (factor >= 1) 
		return vB;

	return (((vB-vA) * factor) + vA);
}

/// return the projection factor from *this to the line determined by points vA and vB
template<class T>
inline T vec2<T>::projectionOnLine(const vec2 &vA, const vec2 &vB) const {
	vec2 v(vB - vA);
	return v.dot(*this - vA) / v.dot(v);
}

/// linear interpolation between 2 vectors
template<class T>
inline vec2<T> vec2<T>::lerp(vec2 &u, vec2 &v, T factor) const { 
	return ((u * (1 - factor)) + (v * factor)); 
}

/// linear interpolation between 2 vectors based on separate x and y factors
template<class T>
inline vec2<T> vec2<T>::lerp(vec2 &u, vec2 &v, vec2& factor) const { 
	return (vec2((u.x * (1 - factor.x)) + (v.x * factor.x), 
				 (u.y * (1 - factor.y)) + (v.y * factor.y))); 
}


/// get the dot product between this vector and the specified one
template<class T>
inline T vec2<T>::dot(const vec2 &v) const { 
	return ((this->x*v.x) + (this->y*v.y)); 
}

template<class T>
inline vec2<T>::vec2(const vec3<T> &v) {
	this->x = v.x;
	this->y = v.y;
}

template<class T>
inline vec2<T>::vec2(const vec4<T> &v) {
	this->x = v.x;
	this->y = v.y;
}

/*
*  vec3 inline definitions
*/
/// compare 2 vectors within the specified tolerance
template<class T>
inline bool vec3<T>::compare(const vec3 &v,F32 epsi=EPSILON) const {
	return FLOAT_COMPARE_TOLERANCE(this->x,v.x,epsi) && 
	 	   FLOAT_COMPARE_TOLERANCE(this->y,v.y,epsi) && 
		   FLOAT_COMPARE_TOLERANCE(this->z,v.z,epsi); 
}

/// uniform vector: x = y = z
template<class T>
inline bool vec3<T>::isUniform() const { 
	return IS_ZERO(this->x - this->y) && IS_ZERO(this->y - this->z);
}

/// return the squared distance of the vector
template<class T>
inline T vec3<T>::lengthSquared() const {
	return this->x * this->x + this->y * this->y + this->z * this->z;
}

/// transform the vector to unit length
template<class T>
inline T vec3<T>::normalize() {
	T l = this->length();

	if(l < EPSILON)
		return 0;

	//multiply by the inverse length
	*this *= (1.0f / l);

	return l;
}

/// set this vector to be equal to the cross of the 2 specified vectors
template<class T>
inline void vec3<T>::cross(const vec3 &v1,const vec3 &v2) {
	this->x = v1.y * v2.z - v1.z * v2.y;
	this->y = v1.z * v2.x - v1.x * v2.z;
	this->z = v1.x * v2.y - v1.y * v2.x;
}

/// set this vector to be equal to the cross between itself and the specified vector
template<class T>
inline void vec3<T>::cross(const vec3 &v2) {
	this->cross(*this, v2);
}

/// calculate the dot product between this vector and the specified one
template<class T>
inline T vec3<T>::dot(const vec3 &v) const { 
	return ((this->x*v.x) + (this->y*v.y) + (this->z*v.z));
}

/// compute the vector's distance to another specified vector
template<class T>
inline T vec3<T>::distance(const vec3 &v) const {
	return square_root_tpl(((v.x - this->x)*(v.x - this->x)) + 
			               ((v.y - this->y)*(v.y - this->y)) + 
						   ((v.z - this->z)*(v.z - this->z)));
}

/// returns the angle in radians between '*this' and 'v'
template<class T>
inline T vec3<T>::angle(vec3 &v) const {
	T angle = (T)fabs(acos(this->dot(v)/(this->length()*v.length())));

	if(angle < EPSILON) 
		return 0;

	return angle;
}

/// get the direction vector to the specified point
template<class T>
inline vec3<T> vec3<T>::direction(const vec3& u) const {
	vec3 vector(u.x - this->x, u.y - this->y, u.z-this->z);
	vector.normalize();
	return vector;
}

/// return the closest point on the line defined by the 2 points (A, B) and this vector
template<class T>
inline vec3<T> vec3<T>::closestPointOnLine(const vec3 &vA, const vec3 &vB) const { 
	return (((vB-vA) * this->projectionOnLine(vA, vB)) + vA); 
}

/// return the closest point on the line segment created between the 2 points (A, B) and this vector
template<class T> 
inline vec3<T> vec3<T>::closestPointOnSegment(const vec3 &vA, const vec3 &vB) const {
	T factor = this->projectionOnLine(vA, vB);

	if (factor <= 0.0f)
		return vA;

	if (factor >= 1.0f)
		return vB;

	return (((vB-vA) * factor) + vA);
}

/// project this vector on the line defined by the 2 points(A, B)
template<class T>
inline T vec3<T>::projectionOnLine(const vec3 &vA, const vec3 &vB) const {
	vec3 vector(vB - vA);
	return vector.dot(*this - vA) / vector.dot(vector);
}

/// lerp between the 2 specified vectors by the specified ammount
template<class T>
inline vec3<T> vec3<T>::lerp(vec3 &u, vec3 &v, T factor) const { 
	return ((u * (1 - factor)) + (v * factor));
}

/// lerp between the 2 specified vectors by the specified ammount for each component
template<class T>
inline vec3<T> vec3<T>::lerp(vec3 &u, vec3 &v, vec3& factor) const {
	return (vec3((u.x * (1 - factor.x)) + (v.x * factor.x),
				 (u.y * (1 - factor.y)) + (v.y * factor.y),
				 (u.z * (1 - factor.z)) + (v.z * factor.z))); 
}

/// rotate this vector on the X axis
template<class T>
inline void vec3<T>::rotateX(D32 radians){
	this->y = (T)( cos(radians)*this->y + sin(radians)*this->z);
    this->z = (T)(-sin(radians)*this->y + cos(radians)*this->z);
}

/// rotate this vector on the Y axis
template<class T>
inline void vec3<T>::rotateY(D32 radians){
	this->x = (T)(cos(radians)*this->x - sin(radians)*this->z);
	this->z = (T)(sin(radians)*this->x + cos(radians)*this->z);
}

/// rotate this vector on the Z axis
template<class T>
inline void vec3<T>::rotateZ(D32 radians){
	this->x = (T)( cos(radians)*this->x + sin(radians)*this->y);
	this->y = (T)(-sin(radians)*this->x + cos(radians)*this->y);
}

/// swap the components  of this vector with that of the specified one
template<class T>
inline void vec3<T>::swap(vec3 &iv) { 
	std::swap(this->x, iv.x);
	std::swap(this->y, iv.y);
	std::swap(this->z, iv.z);
}

/// swap the components  of this vector with that of the specified one
template<class T>
inline void vec3<T>::swap(vec3 *iv) { 
	std::swap(this->x, iv->x);
	std::swap(this->y, iv->y);
	std::swap(this->z, iv->z); 
}

/// export the vector's components in the first 3 positions of the specified array
template<class T>
inline void vec3<T>::get(T * v) const {
	v[0] = (T)this->_v[0];
	v[1] = (T)this->_v[1];
	v[2] = (T)this->_v[2];
}

/// this calculates a vector between the two specified points and returns the result
template<class T>
inline vec3<T> vec3<T>::vector(const vec3 &vp1, const vec3 &vp2) const {
	return vec3(vp1.x - vp2.x, vp1.y - vp2.y, vp1.z - vp2.z);
}

template<class T>
inline vec3<T>::vec3(const vec4<T> &v) {
	this->x = v.x;
	this->y = v.y;
	this->z = v.z;
}

/*
*  vec4 inline definitions
*/

/// compare this vector with the one specified and see if they match within the specified ammount
template<class T>
inline bool vec4<T>::compare(const vec4 &v,F32 epsi=EPSILON) const { 
	return (FLOAT_COMPARE_TOLERANCE((F32)this->x, (F32)v.x, epsi) && 
			FLOAT_COMPARE_TOLERANCE((F32)this->y, (F32)v.y, epsi) && 
			FLOAT_COMPARE_TOLERANCE((F32)this->z, (F32)v.z, epsi) && 
			FLOAT_COMPARE_TOLERANCE((F32)this->w, (F32)v.w, epsi)); 
}

/// lerp between the 2 vectors by the specified ammount
template<class T>
inline vec4<T> vec4<T>::lerp(const vec4 &u, const vec4 &v, T factor) const { 
	return ((u * (1 - factor)) + (v * factor));
}

/// lerp between the 2 specified vectors by the specified ammount for each componet
template<class T>
inline vec4<T> vec4<T>::lerp(vec4 &u, vec4 &v, vec4& factor) const {
	return (vec4((u.x * (1 - factor.x)) + (v.x * factor.x),
				 (u.y * (1 - factor.y)) + (v.y * factor.y),
				 (u.z * (1 - factor.z)) + (v.z * factor.z),
				 (u.w * (1 - factor.w)) + (v.w * factor.w))); 
}

/// swap this vector's values with that of the specified vector
template<class T>
inline void vec4<T>::swap(vec4 *iv) { 
	std::swap(this->x, iv->x);
	std::swap(this->y, iv->y);
	std::swap(this->z, iv->z);
	std::swap(this->w, iv->w);
}

/// swap this vector's values with that of the specified vector
template<class T>
inline void vec4<T>::swap(vec4 &iv) {
	std::swap(this->x, iv.x);
	std::swap(this->y, iv.y);
	std::swap(this->z, iv.z);
	std::swap(this->w, iv.w);
}

/// return the squared distance of the vector
template<class T>
inline T vec4<T>::lengthSquared() const {
	return this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w;
}
/// transform the vector to unit length
template<class T>
inline T vec4<T>::normalize() {
	T l = this->length();

	if(l < EPSILON)
		return 0;

	//multiply by the inverse length
	*this *= (1.0f / l);

	return l;
}

#endif
