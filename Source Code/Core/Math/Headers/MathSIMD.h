/*
   Copyright (c) 2013 DIVIDE-Studio
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

///If we select SIMD for our math operations, we don't limit ourselves to SSE/MMX
///Instead we use ASM where this provides a faster solution (such as the sqrt impl.)
#ifndef MATH_SIMD_H_
#define MATH_SIMD_H_

#include <math.h>
#include <assert.h>
#include <xmmintrin.h>
#include "Hardware/Platform/Headers/PlatformDefines.h"

#define IsAligned(p) ((((unsigned long)(p)) & 15) == 0)
#define NULL    0

	inline D32 __declspec (naked) __fastcall square_root(D32 n){
		_asm fld qword ptr [esp+4]
		_asm fsqrt
		_asm ret 8
	}

	template<class T>
	inline T square_root_tpl(T n){
		return (T)square_root((D32)n);
	}

	static const __m128 one = _mm_set_ps1(1.0f);
	///dot
	__forceinline __m128 _mm_dot_ps(__m128 v1, __m128 v2){
		__m128 mul0 = _mm_mul_ps(v1, v2);
		__m128 swp0 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 3, 0, 1));
		__m128 add0 = _mm_add_ps(mul0, swp0);
		__m128 swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(1, 0, 3, 2));
		__m128 add1 = _mm_add_ps(add0, swp1);
		return add1;
	}

	namespace Mat4{
		///multiply
		__forceinline void _mm_mul_ps(__m128 in1[4], __m128 in2[4], __m128 out[4])
		{
			{
				__m128 e0 = _mm_shuffle_ps(in2[0], in2[0], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 e1 = _mm_shuffle_ps(in2[0], in2[0], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 e2 = _mm_shuffle_ps(in2[0], in2[0], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 e3 = _mm_shuffle_ps(in2[0], in2[0], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 m0 = _mm_mul_ps(in1[0], e0);
				__m128 m1 = _mm_mul_ps(in1[1], e1);
				__m128 m2 = _mm_mul_ps(in1[2], e2);
				__m128 m3 = _mm_mul_ps(in1[3], e3);

				__m128 a0 = _mm_add_ps(m0, m1);
				__m128 a1 = _mm_add_ps(m2, m3);
				__m128 a2 = _mm_add_ps(a0, a1);

				out[0] = a2;
			}

			{
				__m128 e0 = _mm_shuffle_ps(in2[1], in2[1], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 e1 = _mm_shuffle_ps(in2[1], in2[1], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 e2 = _mm_shuffle_ps(in2[1], in2[1], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 e3 = _mm_shuffle_ps(in2[1], in2[1], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 m0 = _mm_mul_ps(in1[0], e0);
				__m128 m1 = _mm_mul_ps(in1[1], e1);
				__m128 m2 = _mm_mul_ps(in1[2], e2);
				__m128 m3 = _mm_mul_ps(in1[3], e3);

				__m128 a0 = _mm_add_ps(m0, m1);
				__m128 a1 = _mm_add_ps(m2, m3);
				__m128 a2 = _mm_add_ps(a0, a1);

				out[1] = a2;
			}

			{
				__m128 e0 = _mm_shuffle_ps(in2[2], in2[2], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 e1 = _mm_shuffle_ps(in2[2], in2[2], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 e2 = _mm_shuffle_ps(in2[2], in2[2], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 e3 = _mm_shuffle_ps(in2[2], in2[2], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 m0 = _mm_mul_ps(in1[0], e0);
				__m128 m1 = _mm_mul_ps(in1[1], e1);
				__m128 m2 = _mm_mul_ps(in1[2], e2);
				__m128 m3 = _mm_mul_ps(in1[3], e3);

				__m128 a0 = _mm_add_ps(m0, m1);
				__m128 a1 = _mm_add_ps(m2, m3);
				__m128 a2 = _mm_add_ps(a0, a1);

				out[2] = a2;
			}

			{
				//(__m128&)_mm_shuffle_epi32(__m128i&)in2[0], _MM_SHUFFLE(3, 3, 3, 3))
				__m128 e0 = _mm_shuffle_ps(in2[3], in2[3], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 e1 = _mm_shuffle_ps(in2[3], in2[3], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 e2 = _mm_shuffle_ps(in2[3], in2[3], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 e3 = _mm_shuffle_ps(in2[3], in2[3], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 m0 = _mm_mul_ps(in1[0], e0);
				__m128 m1 = _mm_mul_ps(in1[1], e1);
				__m128 m2 = _mm_mul_ps(in1[2], e2);
				__m128 m3 = _mm_mul_ps(in1[3], e3);

				__m128 a0 = _mm_add_ps(m0, m1);
				__m128 a1 = _mm_add_ps(m2, m3);
				__m128 a2 = _mm_add_ps(a0, a1);

				out[3] = a2;
			}
		}

		//ToDo: Align memory!!!!
		inline void	Multiply(const F32 * a, const F32 * b, F32 * r)
		{
			assert(a != NULL &&
				   b != NULL &&
				   r != NULL);
			/*assert(IsAligned(a) &&
				   IsAligned(b) &&
				   IsAligned(r));*/

			__m128 a_line, b_line, r_line;
			for (I32 i=0; i<16; i+=4) {
				// unroll the first step of the loop to avoid having to initialize r_line to zero
				a_line = /*_mm_load_ps*/_mm_loadu_ps(a);         // a_line = vec4(column(a, 0))
				b_line = _mm_set1_ps(b[i]);      // b_line = vec4(b[i][0])
				r_line = _mm_mul_ps(a_line, b_line); // r_line = a_line * b_line
				for (I32 j=1; j<4; j++) {
					a_line = /*_mm_load_ps*/_mm_loadu_ps(&a[j*4]); // a_line = vec4(column(a, j))
					b_line = _mm_set1_ps(b[i+j]);  // b_line = vec4(b[i][j])
										 // r_line += a_line * b_line
					r_line = _mm_add_ps(_mm_mul_ps(a_line, b_line), r_line);
				}
				_mm_store_ps(&r[i], r_line);     // r[i] = r_line
			}
		}
		template<class T>
		__forceinline T det(const T* mat) {
			return ((mat[0] * mat[5] * mat[10]) +
					(mat[4] * mat[9] * mat[2])  +
					(mat[8] * mat[1] * mat[6])  -
					(mat[8] * mat[5] * mat[2])  -
					(mat[4] * mat[1] * mat[10]) -
					(mat[0] * mat[9] * mat[6]));
		}

		__forceinline void Inverse(const F32 *src,F32 * dest)
		{
		   __m128 minor0, minor1, minor2, minor3;
		   __m128 row0, row2, det;
		   __m128 row1 = _mm_setzero_ps();
		   __m128 row3 = _mm_setzero_ps();
		   __m128 tmp1 = _mm_setzero_ps();
		   tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src+ 4));
		   row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src+8)), (__m64*)(src+12));
		   row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
		   row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
		   tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src+ 2)), (__m64*)(src+ 6));
		   row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src+10)), (__m64*)(src+14));
		   row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
		   row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);
		   // -----------------------------------------------
		   tmp1 = _mm_mul_ps(row2, row3);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		   minor0 = _mm_mul_ps(row1, tmp1);
		   minor1 = _mm_mul_ps(row0, tmp1);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		   minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
		   minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
		   minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);
		   // -----------------------------------------------
		   tmp1 = _mm_mul_ps(row1, row2);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		   minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
		   minor3 = _mm_mul_ps(row0, tmp1);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

		   minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
		   minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
		   minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);
		   // -----------------------------------------------
		   tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		   row2 = _mm_shuffle_ps(row2, row2, 0x4E);
		   minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
		   minor2 = _mm_mul_ps(row0, tmp1);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		   minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
		   minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
		   minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);
		   // -----------------------------------------------
		   tmp1 = _mm_mul_ps(row0, row1);

		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		   minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
		   minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		   minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
		   minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));
		   // -----------------------------------------------
		   tmp1 = _mm_mul_ps(row0, row3);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		   minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
		   minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		   minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
		   minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));
		   // -----------------------------------------------
		   tmp1 = _mm_mul_ps(row0, row2);
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		   minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
		   minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
		   tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

		   minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
		   minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);
		   // -----------------------------------------------
		   det = _mm_mul_ps(row0, minor0);
		   det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
		   det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
		   tmp1 = _mm_rcp_ss(det);
		   det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
		   det = _mm_shuffle_ps(det, det, 0x00);
		   minor0 = _mm_mul_ps(det, minor0);
		   _mm_storel_pi((__m64*)(dest), minor0);
		   _mm_storeh_pi((__m64*)(dest+2), minor0);
		   minor1 = _mm_mul_ps(det, minor1);
		   _mm_storel_pi((__m64*)(dest+4), minor1);
		   _mm_storeh_pi((__m64*)(dest+6), minor1);
		   minor2 = _mm_mul_ps(det, minor2);
		   _mm_storel_pi((__m64*)(dest+ 8), minor2);
		   _mm_storeh_pi((__m64*)(dest+10), minor2);
		   minor3 = _mm_mul_ps(det, minor3);
		   _mm_storel_pi((__m64*)(dest+12), minor3);
		   _mm_storeh_pi((__m64*)(dest+14), minor3);
		}
	}

#endif