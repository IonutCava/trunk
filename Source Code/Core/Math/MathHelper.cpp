#include "Headers/MathClasses.h"
#include "Headers/Quaternion.h"

namespace Mat4 {
			// ----------------------------------------------------------------------------------------
		template <typename T>
		inline void decompose (const mat4<T>& matrix, vec3<T>& scale, Quaternion<T>& rotation,	vec3<T>& position) {
			const mat4<T>& this_ = matrix;

			// extract translation
			position.x = this_.data[0][3];
			position.y = this_.data[1][3];
			position.z = this_.data[2][3];

			// extract the rows of the matrix
			vec3<T> vRows[3] = {
				vec3<T>(_this.data[0][0],_this.data[1][0],_this.data[2][0]),
				vec3<T>(_this.data[0][1],_this.data[1][1],_this.data[2][1]),
				vec3<T>(_this.data[0][2],_this.data[1][2],_this.data[2][2])
			};

			// extract the scaling factors
			scaling.x = vRows[0].length();
			scaling.y = vRows[1].length();
			scaling.z = vRows[2].length();

			// and the sign of the scaling
			if (det() < 0) {
				scaling.x = -scaling.x;
				scaling.y = -scaling.y;
				scaling.z = -scaling.z;
			}

			// and remove all scaling from the matrix
			if(scaling.x)
			{
				vRows[0] /= scaling.x;
			}
			if(scaling.y)
			{
				vRows[1] /= scaling.y;
			}
			if(scaling.z)
			{
				vRows[2] /= scaling.z;
			}

			// build a 3x3 rotation matrix
			mat3<T> m(vRows[0].x,vRows[1].x,vRows[2].x,
				vRows[0].y,vRows[1].y,vRows[2].y,
				vRows[0].z,vRows[1].z,vRows[2].z);

			// and generate the rotation quaternion from it
			rotation = Quaternion<T>(m);
		}

		// ----------------------------------------------------------------------------------------
		template <typename T>
		inline void decomposeNoScaling(const mat4<T>& matrix, Quaternion<T>& rotation,	vec3<T>& position) {
			const aiMatrix4x4t<TReal>& this_ = *this;

			// extract translation
			position.x = this_.data[0][3];
			position.y = this_.data[1][3];
			position.z = this_.data[2][3];

			// extract rotation
			rotation = Quaterion<T>((mat3<T>)_this);
		}
};