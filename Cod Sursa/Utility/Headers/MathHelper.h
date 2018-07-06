#ifndef _MATH_HELPER_H_
#define _MATH_HELPER_H_

#include "resource.h"

namespace Util
{
	template<class T>
	string toString(T data)
	{
		stringstream s;
		s << data;
		return s.str();
	}

}

#endif