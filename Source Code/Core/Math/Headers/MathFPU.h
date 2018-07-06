/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef MATH_FPU_H_
#define MATH_FPU_H_

#include <math.h>
#include "Hardware/Platform/Headers/PlatformDefines.h"

	inline D32 square_root(D32 n){
		return sqrt(n);
	}

	template<class T>
	inline T square_root_tpl(T n){
		return (T)square_root((D32)n);
	}

namespace Mat4{
	inline void Multiply(const F32 *a, const F32 *b, F32 *r){
	    U32 row, column, row_offset;
	    for (row = 0, row_offset = row * 4; row < 4; ++row, row_offset = row * 4){
	        for (column = 0; column < 4; ++column){
	            r[row_offset + column] =
	                (a[row_offset + 0] * b[column + 0]) +
	                (a[row_offset + 1] * b[column + 4]) +
	                (a[row_offset + 2] * b[column + 8]) +
	                (a[row_offset + 3] * b[column + 12]);
			}
		}
	 /*
		for (I32 i=0; i<16; i+=4){
			for (I32 j=0; j<4; j++){
				r[i+j] = b[i]*a[j] + b[i+1]*a[j+4] + b[i+2]*a[j+8] + b[i+3]*a[j+12];
			}
		}
		*/
	}
	__forceinline F32 det(const F32* mat) {
		return ((mat[0] * mat[5] * mat[10]) +
				(mat[4] * mat[9] * mat[2])  +
				(mat[8] * mat[1] * mat[6])  -
				(mat[8] * mat[5] * mat[2])  -
				(mat[4] * mat[1] * mat[10]) -
				(mat[0] * mat[9] * mat[6]));
	}

	__forceinline void Inverse(const F32* in,F32* out){
		F32 idet = 1.0f / det(in);
		out[0]  =  (in[5] * in[10] - in[9] * in[6]) * idet;
		out[1]  = -(in[1] * in[10] - in[9] * in[2]) * idet;
		out[2]  =  (in[1] * in[6]  - in[5] * in[2]) * idet;
		out[3]  = 0.0f;
		out[4]  = -(in[4] * in[10] - in[8] * in[6]) * idet;
		out[5]  =  (in[0] * in[10] - in[8] * in[2]) * idet;
		out[6]  = -(in[0] * in[6]  - in[4] * in[2]) * idet;
		out[7]  = 0.0f;
		out[8]  =  (in[4] * in[9] - in[8] * in[5]) * idet;
		out[9]  = -(in[0] * in[9] - in[8] * in[1]) * idet;
		out[10] =  (in[0] * in[5] - in[4] * in[1]) * idet;
		out[11] = 0.0f;
		out[12] = -(in[12] * (out)[0] + in[13] * (out)[4] + in[14] * (out)[8]);
		out[13] = -(in[12] * (out)[1] + in[13] * (out)[5] + in[14] * (out)[9]);
		out[14] = -(in[12] * (out)[2] + in[13] * (out)[6] + in[14] * (out)[10]);
		out[15] = 1.0f;
	}
}
#endif