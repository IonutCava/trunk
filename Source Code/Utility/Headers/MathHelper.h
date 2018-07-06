/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#include "resource.h"
#include <xmmintrin.h>
namespace Util {

	template<class T>
	inline std::string toString(T data){
		std::stringstream s;
		s << data;
		return s.str();
	}
#if defined  USE_MATH_SSE		
	static const __m128 one = _mm_set_ps1(1.0f);
	//dot
	__forceinline __m128 _mm_dot_ps(__m128 v1, __m128 v2){
		__m128 mul0 = _mm_mul_ps(v1, v2);
		__m128 swp0 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 3, 0, 1));
		__m128 add0 = _mm_add_ps(mul0, swp0);
		__m128 swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(1, 0, 3, 2));
		__m128 add1 = _mm_add_ps(add0, swp1);
		return add1;
	}

	inline D32 __declspec (naked) __fastcall square_root(D32 n){
	
		_asm fld qword ptr [esp+4]
		_asm fsqrt
		_asm ret 8
	} 

	namespace Mat4{
		//multiply
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

		inline void
		mmul(const float * a, const float * b, float * r)
		{
			__m128 a_line, b_line, r_line;
			for (int i=0; i<16; i+=4) {
				// unroll the first step of the loop to avoid having to initialize r_line to zero
				a_line = _mm_load_ps(a);         // a_line = vec4(column(a, 0))
				b_line = _mm_set1_ps(b[i]);      // b_line = vec4(b[i][0])
				r_line = _mm_mul_ps(a_line, b_line); // r_line = a_line * b_line
				for (int j=1; j<4; j++) {
					a_line = _mm_load_ps(&a[j*4]); // a_line = vec4(column(a, j))
					b_line = _mm_set1_ps(b[i+j]);  // b_line = vec4(b[i][j])
										 // r_line += a_line * b_line
					r_line = _mm_add_ps(_mm_mul_ps(a_line, b_line), r_line);
				}
				_mm_store_ps(&r[i], r_line);     // r[i] = r_line
			}
		}
		__forceinline void _mm_inverse_ps(const float *a,float * r)
		{
			__m128 in[4];
			in[0] = _mm_loadu_ps(a+0);
			in[1] = _mm_loadu_ps(a+1);
			in[2] = _mm_loadu_ps(a+2);
			in[3] = _mm_loadu_ps(a+3);
			__m128 Fac0;
			{
				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac0 = _mm_sub_ps(Mul00, Mul01);

				bool stop = true;
			}

			__m128 Fac1;
			{

				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac1 = _mm_sub_ps(Mul00, Mul01);

				bool stop = true;
			}


			__m128 Fac2;
			{

				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac2 = _mm_sub_ps(Mul00, Mul01);

				bool stop = true;
			}

			__m128 Fac3;
			{
				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac3 = _mm_sub_ps(Mul00, Mul01);

				bool stop = true;
			}

			__m128 Fac4;
			{
				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac4 = _mm_sub_ps(Mul00, Mul01);

				bool stop = true;
			}

			__m128 Fac5;
			{
				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac5 = _mm_sub_ps(Mul00, Mul01);

				bool stop = true;
			}

			__m128 SignA = _mm_set_ps( 1.0f,-1.0f, 1.0f,-1.0f);
			__m128 SignB = _mm_set_ps(-1.0f, 1.0f,-1.0f, 1.0f);

			__m128 Temp0 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Vec0 = _mm_shuffle_ps(Temp0, Temp0, _MM_SHUFFLE(2, 2, 2, 0));
		
			__m128 Temp1 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Vec1 = _mm_shuffle_ps(Temp1, Temp1, _MM_SHUFFLE(2, 2, 2, 0));

			__m128 Temp2 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Vec2 = _mm_shuffle_ps(Temp2, Temp2, _MM_SHUFFLE(2, 2, 2, 0));

			__m128 Temp3 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Vec3 = _mm_shuffle_ps(Temp3, Temp3, _MM_SHUFFLE(2, 2, 2, 0));

			__m128 Mul00 = _mm_mul_ps(Vec1, Fac0);
			__m128 Mul01 = _mm_mul_ps(Vec2, Fac1);
			__m128 Mul02 = _mm_mul_ps(Vec3, Fac2);
			__m128 Sub00 = _mm_sub_ps(Mul00, Mul01);
			__m128 Add00 = _mm_add_ps(Sub00, Mul02);
			__m128 Inv0 = _mm_mul_ps(SignB, Add00);

			__m128 Mul03 = _mm_mul_ps(Vec0, Fac0);
			__m128 Mul04 = _mm_mul_ps(Vec2, Fac3);
			__m128 Mul05 = _mm_mul_ps(Vec3, Fac4);
			__m128 Sub01 = _mm_sub_ps(Mul03, Mul04);
			__m128 Add01 = _mm_add_ps(Sub01, Mul05);
			__m128 Inv1 = _mm_mul_ps(SignA, Add01);

			__m128 Mul06 = _mm_mul_ps(Vec0, Fac1);
			__m128 Mul07 = _mm_mul_ps(Vec1, Fac3);
			__m128 Mul08 = _mm_mul_ps(Vec3, Fac5);
			__m128 Sub02 = _mm_sub_ps(Mul06, Mul07);
			__m128 Add02 = _mm_add_ps(Sub02, Mul08);
			__m128 Inv2 = _mm_mul_ps(SignB, Add02);

			__m128 Mul09 = _mm_mul_ps(Vec0, Fac2);
			__m128 Mul10 = _mm_mul_ps(Vec1, Fac4);
			__m128 Mul11 = _mm_mul_ps(Vec2, Fac5);
			__m128 Sub03 = _mm_sub_ps(Mul09, Mul10);
			__m128 Add03 = _mm_add_ps(Sub03, Mul11);
			__m128 Inv3 = _mm_mul_ps(SignA, Add03);

			__m128 Row0 = _mm_shuffle_ps(Inv0, Inv1, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Row1 = _mm_shuffle_ps(Inv2, Inv3, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Row2 = _mm_shuffle_ps(Row0, Row1, _MM_SHUFFLE(2, 0, 2, 0));
		
			__m128 Det0 = _mm_dot_ps(in[0], Row2);
			__m128 Rcp0 = _mm_div_ps(one, Det0);

			_mm_store_ps(&r[0], _mm_mul_ps(Inv0, Rcp0));
			_mm_store_ps(&r[1], _mm_mul_ps(Inv1, Rcp0));
			_mm_store_ps(&r[2], _mm_mul_ps(Inv2, Rcp0));
			_mm_store_ps(&r[3], _mm_mul_ps(Inv3, Rcp0));
		}
	}
#else
	inline D32 square_root(D32 n){
		return sqrtf(n);
	}

	namespace Mat4{
		inline void mmul(const float *a, const float *b, float *r){
			for (int i=0; i<16; i+=4){
				for (int j=0; j<4; j++){
					r[i+j] = b[i]*a[j] + b[i+1]*a[j+4] + b[i+2]*a[j+8] + b[i+3]*a[j+12];
				}
			}
		}
		__forceinline F32 det(const float* mat) {
			return ((mat[0] * mat[5] * mat[10]) +
					(mat[4] * mat[9] * mat[2])  +
					(mat[8] * mat[1] * mat[6])  -
					(mat[8] * mat[5] * mat[2])  -
					(mat[4] * mat[1] * mat[10]) -
					(mat[0] * mat[9] * mat[6]));
		}

		__forceinline void _mm_inverse_ps(const float* in,float* out){
			F32 idet = 1.0f / det(in);
			out[0]  =  (in[5] * in[10] - in[9] * in[6]) * idet;
			out[1]  = -(in[1] * in[10] - in[9] * in[2]) * idet;
			out[2]  =  (in[1] * in[6]  - in[5] * in[2]) * idet;
			out[3]  = 0.0;
			out[4]  = -(in[4] * in[10] - in[8] * in[6]) * idet;
			out[5]  =  (in[0] * in[10] - in[8] * in[2]) * idet;
			out[6]  = -(in[0] * in[6]  - in[4] * in[2]) * idet;
			out[7]  = 0.0;
			out[8]  =  (in[4] * in[9] - in[8] * in[5]) * idet;
			out[9]  = -(in[0] * in[9] - in[8] * in[1]) * idet;
			out[10] =  (in[0] * in[5] - in[4] * in[1]) * idet;
			out[11] = 0.0;
			out[12] = -(in[12] * (out)[0] + in[13] * (out)[4] + in[14] * (out)[8]);
			out[13] = -(in[12] * (out)[1] + in[13] * (out)[5] + in[14] * (out)[9]);
			out[14] = -(in[12] * (out)[2] + in[13] * (out)[6] + in[14] * (out)[10]);
			out[15] = 1.0;

		}
	}
#endif

}

#endif