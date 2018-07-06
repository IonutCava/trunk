#ifndef _MATH_HELPER_H_
#define _MATH_HELPER_H_

#include "resource.h"

namespace Util
{
	template<class T>
	std::string toString(T data)
	{
		std::stringstream s;
		s << data;
		return s.str();
	}

}

#endif