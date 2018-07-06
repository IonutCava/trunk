#include "stdafx.h"

#include "Headers/MathMatrices.h"

namespace Divide {

vec2<F32> VECTOR2_ZERO = vec2<F32>(0.0f);
vec3<F32> VECTOR3_ZERO = vec3<F32>(0.0f);
vec4<F32> VECTOR4_ZERO = vec4<F32>(0.0f);
vec2<F32> VECTOR2_UNIT = vec2<F32>(1.0f);
vec3<F32> VECTOR3_UNIT = vec3<F32>(1.0f);
vec4<F32> VECTOR4_UNIT = vec4<F32>(1.0f);
vec3<F32> WORLD_X_AXIS = vec3<F32>(1.0f, 0.0f, 0.0f);
vec3<F32> WORLD_Y_AXIS = vec3<F32>(0.0f, 1.0f, 0.0f);
vec3<F32> WORLD_Z_AXIS = vec3<F32>(0.0f, 0.0f, 1.0f);
vec3<F32> WORLD_X_NEG_AXIS = vec3<F32>(-1.0f, 0.0f, 0.0f);
vec3<F32> WORLD_Y_NEG_AXIS = vec3<F32>(0.0f, -1.0f, 0.0f);
vec3<F32> WORLD_Z_NEG_AXIS = vec3<F32>(0.0f, 0.0f, -1.0f);
vec3<F32> DEFAULT_GRAVITY = vec3<F32>(0.0f, -9.81f, 0.0f);
vec4<F32> UNIT_RECT = vec4<F32>(-1.0f, 1.0f, -1.0f, 1.0f);

vec2<I32> iVECTOR2_ZERO = vec2<I32>(0);
vec3<I32> iVECTOR3_ZERO = vec3<I32>(0);
vec4<I32> iVECTOR4_ZERO = vec4<I32>(0);
vec3<I32> iWORLD_X_AXIS = vec3<I32>(1, 0, 0);
vec3<I32> iWORLD_Y_AXIS = vec3<I32>(0, 1, 0);
vec3<I32> iWORLD_Z_AXIS = vec3<I32>(0, 0, 1);
vec3<I32> iWORLD_X_NEG_AXIS = vec3<I32>(-1, 0, 0);
vec3<I32> iWORLD_Y_NEG_AXIS = vec3<I32>(0, -1, 0);
vec3<I32> iWORLD_Z_NEG_AXIS = vec3<I32>(0, 0, -1);


mat2<F32> MAT2_BIAS(0.5, 0.0,
                    0.0, 0.5);

mat3<F32> MAT3_BIAS(0.5, 0.0, 0.0,
                    0.0, 0.5, 0.0,
                    0.0, 0.0, 0.5);

mat4<F32> MAT4_BIAS(0.5, 0.0, 0.0, 0.0,
                    0.0, 0.5, 0.0, 0.0,
                    0.0, 0.0, 0.5, 0.0,
                    0.5, 0.5, 0.5, 1.0);

mat2<F32> MAT2_IDENTITY;
mat3<F32> MAT3_IDENTITY;
mat4<F32> MAT4_IDENTITY;
};  // namespace Divide