#ifndef BOUNDINGBOX_H_
#define BOUNDINGBOX_H_
#include "MathClasses.h"
#include "Utility/Headers/Ray.h"
//ToDo: -Add BoundingSphere -Ionut
class BoundingBox
{
public:
	BoundingBox() {min.reset(); max.reset();_computed = false;_visibility = false;}
	BoundingBox(const vec3& _min, const vec3& _max) {min=_min; max=_max;_computed = false;_visibility = false;}

	inline bool ContainsPoint(const vec3& point) const	{
		return (point.x>=min.x && point.y>=min.y && point.z>=min.z && point.x<=max.x && point.y<=max.y && point.z<=max.z);
	};

	bool  Collision(const BoundingBox& AABB2)
	{

		if(max.x < AABB2.min.x) return false;
		if(max.y < AABB2.min.y) return false;
		if(max.z < AABB2.min.z) return false;

		if(min.x > AABB2.max.x) return false;
   		if(min.y > AABB2.max.y) return false;
		if(min.z > AABB2.max.z) return false;

        return true;
	}

	// Optimized method
	inline bool intersect(const Ray &r, F32 t0, F32 t1) const {
		float tmin, tmax, tymin, tymax, tzmin, tzmax;
		vec3 bounds[2];
		bounds[0] = min; bounds[1] = max;

		tmin = (bounds[r.sign[0]].x - r.origin.x) * r.inv_direction.x;
		tmax = (bounds[1-r.sign[0]].x - r.origin.x) * r.inv_direction.x;
		tymin = (bounds[r.sign[1]].y - r.origin.y) * r.inv_direction.y;
		tymax = (bounds[1-r.sign[1]].y - r.origin.y) * r.inv_direction.y;

		if ( (tmin > tymax) || (tymin > tmax) ) 
			return false;

		if (tymin > tmin)
			tmin = tymin;
		if (tymax < tmax)
			tmax = tymax;
		tzmin = (bounds[r.sign[2]].z - r.origin.z) * r.inv_direction.z;
		tzmax = (bounds[1-r.sign[2]].z - r.origin.z) * r.inv_direction.z;
		
		if ( (tmin > tzmax) || (tzmin > tmax) )
			return false;

		if (tzmin > tmin)
			tmin = tzmin;
		if (tzmax < tmax)
			tmax = tzmax;

		return ( (tmin < t1) && (tmax > t0) );
}
	
	inline void Add(const vec3& v)	{
		if(v.x > max.x)	max.x = v.x;
		if(v.x < min.x)	min.x = v.x;
		if(v.y > max.y)	max.y = v.y;
		if(v.y < min.y)	min.y = v.y;
		if(v.z > max.z)	max.z = v.z;
		if(v.z < min.z)	min.z = v.z;
	};

	inline void Add(const BoundingBox& bb)	{
		if(bb.max.x > max.x)	max.x = bb.max.x;
		if(bb.min.x < min.x)	min.x = bb.min.x;
		if(bb.max.y > max.y)	max.y = bb.max.y;
		if(bb.min.y < min.y)	min.y = bb.min.y;
		if(bb.max.z > max.z)	max.z = bb.max.z;
		if(bb.min.z < min.z)	min.z = bb.min.z;
	}

	inline void Translate(const vec3& v) {
		min += v;
		max += v;
	}

	inline void Multiply(const vec3& v){
		min.x *= v.x;
		min.y *= v.y;
		min.z *= v.z;
		max.x *= v.x;
		max.y *= v.y;
		max.z *= v.z;
	}
	inline void MultiplyMax(const vec3& v){
		max.x *= v.x;
		max.y *= v.y;
		max.z *= v.z;
	}
	inline void MultiplyMin(const vec3& v){
		min.x *= v.x;
		min.y *= v.y;
		min.z *= v.z;
	}

	void transform(const mat4 &mat)
	{
		float av, bv;
		int   i, j;
		float m[4][4];
	    m[0][0] = mat[0]; m[1][0] = mat[1]; m[2][0] = mat[2];  m[3][0] = mat[3];
		m[0][1] = mat[4]; m[1][1] = mat[5]; m[2][1] = mat[6];  m[3][1] = mat[7];
		m[0][2] = mat[8]; m[1][2] = mat[9]; m[2][2] = mat[10]; m[3][2] = mat[11];
		m[0][3] = mat[12]; m[1][3] = mat[13]; m[2][3] = mat[14]; m[3][3] = mat[15];
			
		BoundingBox new_box(vec3(mat[12], mat[13], mat[14]),vec3(mat[12], mat[13], mat[14]));
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
			{
				av = m[i][j] * min[j];
				bv = m[i][j] * max[j];
				if (av < bv)
				{
					new_box.min[i] += av;
					new_box.max[i] += bv;
				} else {
					new_box.min[i] += bv;
					new_box.max[i] += av;
				}
			}

		min = new_box.min;		
		max = new_box.max;
	}

	
	inline bool& isComputed() {return _computed;}
	inline bool  getVisibility() {return _visibility;}

	inline vec3  getCenter()     const	{return (max+min)/2.0f;}
	inline vec3  getExtent()     const  {return (max-min);}
	inline vec3  getHalfExtent() const  {return getExtent()/2.0f;}

		   F32   getWidth()  const {return max.x - min.x;}
		   F32   getHeight() const {return max.y - min.y;}
		   F32   getDepth()  const {return max.z - min.z;}

	void setVisibility(bool visibility) {_visibility = visibility;}

	vec3 min, max;

private:
	bool _computed, _visibility;
};

#endif


