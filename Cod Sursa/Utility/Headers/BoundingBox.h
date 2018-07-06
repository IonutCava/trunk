#ifndef BOUNDINGBOX_H_
#define BOUNDINGBOX_H_
#include "MathClasses.h"
#include "Utility/Headers/Ray.h"
#include "Utility/Headers/Transform.h"
//ToDo: -Add BoundingSphere -Ionut
class BoundingBox
{
public:
	BoundingBox() 
	{
		_min.reset();
		_max.reset();
		_computed = false;
		_visibility = false;
		points = new vec3[8];
	}

	BoundingBox(const vec3& __min, const vec3& __max)
	{
		_min=__min;
		_max=__max;
		_computed = false;
		_visibility = false;
		points = new vec3[8];
	}
	~BoundingBox(){	delete points; }

	inline bool ContainsPoint(const vec3& point) const	{
		return (point.x>=_min.x && point.y>=_min.y && point.z>=_min.z && point.x<=_max.x && point.y<=_max.y && point.z<=_max.z);
	};

	bool  Collision(const BoundingBox& AABB2)
	{

		if(_max.x < AABB2._min.x) return false;
		if(_max.y < AABB2._min.y) return false;
		if(_max.z < AABB2._min.z) return false;

		if(_min.x > AABB2._max.x) return false;
   		if(_min.y > AABB2._max.y) return false;
		if(_min.z > AABB2._max.z) return false;

        return true;
	}

	// Optimized method
	inline bool intersect(const Ray &r, F32 t0, F32 t1) const {
		F32 t_min, t_max, ty_min, ty_max, tz_min, tz_max;
		vec3 bounds[2];
		bounds[0] = _min; bounds[1] = _max;

		t_min = (bounds[r.sign[0]].x - r.origin.x) * r.inv_direction.x;
		t_max = (bounds[1-r.sign[0]].x - r.origin.x) * r.inv_direction.x;
		ty_min = (bounds[r.sign[1]].y - r.origin.y) * r.inv_direction.y;
		ty_max = (bounds[1-r.sign[1]].y - r.origin.y) * r.inv_direction.y;

		if ( (t_min > ty_max) || (ty_min > t_max) ) 
			return false;

		if (ty_min > t_min)
			t_min = ty_min;
		if (ty_max < t_max)
			t_max = ty_max;
		tz_min = (bounds[r.sign[2]].z - r.origin.z) * r.inv_direction.z;
		tz_max = (bounds[1-r.sign[2]].z - r.origin.z) * r.inv_direction.z;
		
		if ( (t_min > tz_max) || (tz_min > t_max) )
			return false;

		if (tz_min > t_min)
			t_min = tz_min;
		if (tz_max < t_max)
			t_max = tz_max;

		return ( (t_min < t1) && (t_max > t0) );
}
	
	inline void Add(const vec3& v)	{
		if(v.x > _max.x)	_max.x = v.x;
		if(v.x < _min.x)	_min.x = v.x;
		if(v.y > _max.y)	_max.y = v.y;
		if(v.y < _min.y)	_min.y = v.y;
		if(v.z > _max.z)	_max.z = v.z;
		if(v.z < _min.z)	_min.z = v.z;
	};

	inline void Add(const BoundingBox& bb)	{
		if(bb._max.x > _max.x)	_max.x = bb._max.x;
		if(bb._min.x < _min.x)	_min.x = bb._min.x;
		if(bb._max.y > _max.y)	_max.y = bb._max.y;
		if(bb._min.y < _min.y)	_min.y = bb._min.y;
		if(bb._max.z > _max.z)	_max.z = bb._max.z;
		if(bb._min.z < _min.z)	_min.z = bb._min.z;
	}

	inline void Translate(const vec3& v) {
		_min += v;
		_max += v;
	}

	inline void Multiply(const vec3& v){
		_min.x *= v.x;
		_min.y *= v.y;
		_min.z *= v.z;
		_max.x *= v.x;
		_max.y *= v.y;
		_max.z *= v.z;
	}

	inline void MultiplyMax(const vec3& v){
		_max.x *= v.x;
		_max.y *= v.y;
		_max.z *= v.z;
	}
	inline void MultiplyMin(const vec3& v){
		_min.x *= v.x;
		_min.y *= v.y;
		_min.z *= v.z;
	}

	bool ComputePoints()  const
	{
		if(!points)	return false;

		points[0] = vec3(_min.x, _min.y, _min.z);
		points[1] = vec3(_max.x, _min.y, _min.z);
		points[2] = vec3(_max.x, _max.y, _min.z);
		points[3] = vec3(_min.x, _max.y, _min.z);
		points[4] = vec3(_min.x, _min.y, _max.z);
		points[5] = vec3(_max.x, _min.y, _max.z);
		points[6] = vec3(_max.x, _max.y, _max.z);
		points[7] = vec3(_min.x, _max.y, _max.z);

		return true;
	}

	void Transform(BoundingBox originalBB, const mat4& mat)
	{
		if(_oldMatrix == mat) return;
		else _oldMatrix = mat;

		F32 a, b;
		vec3 old_min = originalBB.getMin();
		vec3 old_max = originalBB.getMax();
		_min = _max =  vec3(mat[12],mat[13],mat[14]);

		for (U32 i = 0; i < 3; ++i)
		{
			for (U32 j = 0; j < 3; ++j)
			{
				//calculate a new AABB for only 90% of the resulting OBB
				//Makes for a tighter fit
				a = mat.element(i,j) * old_min[j] * 0.90f;
				b = mat.element(i,j) * old_max[j] * 0.90f;

				if (a < b)
				{
					_min[i] += a;
					_max[i] += b;
				} else {
					_min[i] += b;
					_max[i] += a;
				}
			}
		}
		
	}


	inline bool& isComputed()		    {return _computed;}
	inline bool  getVisibility()		{return _visibility;}

	inline vec3  getMin()	     const	{return _min;}
	inline vec3  getMax()		 const	{return _max;}
	inline vec3  getCenter()     const	{return (_max+_min)/2.0f;}
	inline vec3  getExtent()     const  {return (_max-_min);}
	inline vec3  getHalfExtent() const  {return getExtent()/2.0f;}

		   vec3* getPoints()     const  {return points;}
		   F32   getWidth()  const {return _max.x - _min.x;}
		   F32   getHeight() const {return _max.y - _min.y;}
		   F32   getDepth()  const {return _max.z - _min.z;}
	

	void setVisibility(bool visibility) {_visibility = visibility;}
	void setMin(vec3& min)			    {_min = min;}
	void setMax(vec3& max)			    {_max = max;}


private:
	bool _computed, _visibility;
	vec3* points;
	vec3 _min, _max;
	mat4 _oldMatrix;
};

#endif


