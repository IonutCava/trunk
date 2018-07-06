#pragma once
#include "resource.h"

namespace Math
{

	void invertMatrix(const GLdouble *m, GLdouble *out );
	D32 vlen(D32 x,D32 y,D32 z);
	void addVector3(F32 *a, F32 *b);

	template <class T>
	inline T clamp(T x, T min, T max)
	{ 
		return (x < min) ? min : (x > max) ? max : x;
	}

	inline int round(F32 f)
	{
		return int(f + 0.5f);
	}
}