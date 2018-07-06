/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

/*Code references:
	Matrix inverse: http://www.devmaster.net/forums/showthread.php?t=14569
	Matrix multiply:  http://fhtr.blogspot.com/2010/02/4x4-float-matrix-multiplication-using.html
	Square root: http://www.codeproject.com/KB/cpp/Sqrt_Prec_VS_Speed.aspx
*/

#ifndef _MATH_HELPER_H_
#define _MATH_HELPER_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include <string>

namespace Util {

	template <typename T>
	class mat4;
	template <typename T>
	class vec3;
	template <typename T>
	class Quaternion;

	template<class T>
	inline std::string toString(const T& data){
		std::stringstream s;
		s << data;
		return s.str();
	}
	inline F32 max(const F32& a, const F32& b){
		return (a<b) ? b : a;
	}

	inline F32 min(const F32& a, const F32& b){
		return (a<b) ? a : b;
	}

	namespace Mat4{
		// ----------------------------------------------------------------------------------------
		template <typename T>
		void decompose (const mat4<T>& matrix, vec3<T>& scale, Quaternion<T>& rotation,	vec3<T>& position);
		template <typename T>
		void decomposeNoScaling(const mat4<T>& matrix, Quaternion<T>& rotation,	vec3<T>& position);
	}
}

#endif




