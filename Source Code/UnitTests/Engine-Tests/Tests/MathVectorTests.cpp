#include "Headers/Defines.h"

#include "Core/Math/Headers/MathVectors.h"

namespace Divide {

TEST_CONST_MEMBER_FUNCTION(vecN, vecN, NA)
{
    const vec2<F32> input1(1.0f, 2.0f);
    const vec2<I32> input2(1, 2);

    CHECK_EQUAL(input2, vec2<I32>(input1));

    CHECK_EQUAL(vec2<F32>(vec3<I32>(vec4<U32>(5.0f))), vec2<F32>(vec3<U32>(vec4<I32>(5u))));
}

TEST_CONST_MEMBER_FUNCTION(vecN, length, NA)
{
    vec2<F32> input1;
    vec3<F32> input2;
    vec4<F32> input3;

    CHECK_ZERO(input1.length());
    CHECK_ZERO(input2.length());
    CHECK_EQUAL(input3.length(), 1);
    input3.w = 0.0f;
    CHECK_ZERO(input3.length());


    input1.set(2.0f, 3.0f);
    CHECK_NOT_ZERO(input1.length());

    input2.set(4.0f, 3.0f, 2.0f);
    input3.set(4.0f, 3.0f, 2.0f, 0.0f);
    CHECK_EQUAL(std::sqrt(input2.lengthSquared()), input3.length());
}

TEST_CONST_MEMBER_FUNCTION(vecN, mul, coef)
{
    const vec2<I32> input1(-2);
    const vec3<F32> input2(5.0f, 0.0f, -5.0f);
    const vec4<U32> input3(10);

    const vec2<I32> result1(-22);
    const vec3<F32> result2(2.5f, 0.0f, -2.5f);
    const vec4<U32> result3(30);

    CHECK_EQUAL(input1 * 11, result1);
    CHECK_EQUAL(input2 * 0.5f, result2);
    CHECK_EQUAL(input3 * 3, result3);
}

TEST_CONST_MEMBER_FUNCTION(vecN, dot, vecN)
{
    const vec2<U32> input1(2);
    const vec3<I32> input2(5, 0, -5);
    const vec4<F32> input3(10.0f);

    const vec2<U32> input4(4);
    const vec3<I32> input5(2, 3, -1);
    const vec4<F32> input6(1.0f);

    CHECK_EQUAL(input1.dot(input4), (2u * 4u) + (2u * 4u));
    CHECK_EQUAL(input2.dot(input5), (5 * 2) +( 0 * 3) + (-5 * -1));
    CHECK_EQUAL(input3.dot(input6), (10.0f * 1.0f) + (10.0f * 1.0f) + (10.0f * 1.0f) + (10.0f * 1.0f));
}

TEST_CONST_MEMBER_FUNCTION(vecN, mul, vecN)
{
    const vec2<U32> input1(2);
    const vec3<I32> input2(5, 0, -5);
    const vec4<F32> input3(10.0f);

    const vec2<U32> input4(4);
    const vec3<I32> input5(2, 3, -1);
    const vec4<F32> input6(1.0f);

    const vec2<U32> result1((2u * 4u), (2u * 4u));
    const vec3<I32> result2((5 * 2), (0 * 3), (-5 * -1));
    const vec4<F32> result3((10.0f * 1.0f), (10.0f * 1.0f), (10.0f * 1.0f), (10.0f * 1.0f));

    CHECK_EQUAL(input1 * input4, result1);
    CHECK_EQUAL(input2 * input5, result2);
    CHECK_EQUAL(input3 * input6, result3);
}

};