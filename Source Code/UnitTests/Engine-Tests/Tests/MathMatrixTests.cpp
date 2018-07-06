#include "Headers/Defines.h"
#include "Core/Math/Headers/MathMatrices.h"

namespace Divide {
// make sure mat4 test include separe floating point and integer calls
// floating point mat4 uses SSE for performance reasons and results might differ

TEST(matNSizeTest)
{
    mat2<I8>  a1;
    mat2<U8>  a2;
    mat2<I16> a3;
    mat2<U16> a4;
    mat2<I32> a5;
    mat2<U32> a6;
    mat2<I64> a7;
    mat2<U64> a8;
    mat2<F32> a9;
    mat2<D64> a10;

    CHECK_EQUAL(sizeof(a5), (32 * 2 * 2) / 8);

    CHECK_EQUAL(sizeof(a7), sizeof(vec2<I64>) * 2);

    CHECK_EQUAL(sizeof(a1),  sizeof(I8)  * 2 * 2);
    CHECK_EQUAL(sizeof(a2),  sizeof(U8)  * 2 * 2);
    CHECK_EQUAL(sizeof(a3),  sizeof(I16) * 2 * 2);
    CHECK_EQUAL(sizeof(a4),  sizeof(U16) * 2 * 2);
    CHECK_EQUAL(sizeof(a5),  sizeof(I32) * 2 * 2);
    CHECK_EQUAL(sizeof(a6),  sizeof(U32) * 2 * 2);
    CHECK_EQUAL(sizeof(a7),  sizeof(I64) * 2 * 2);
    CHECK_EQUAL(sizeof(a8),  sizeof(U64) * 2 * 2);
    CHECK_EQUAL(sizeof(a9),  sizeof(F32) * 2 * 2);
    CHECK_EQUAL(sizeof(a10), sizeof(D64) * 2 * 2);
    CHECK_EQUAL(sizeof(a10), sizeof(a9)  * 2);

    mat3<I8>  b1;
    mat3<U8>  b2;
    mat3<I16> b3;
    mat3<U16> b4;
    mat3<I32> b5;
    mat3<U32> b6;
    mat3<I64> b7;
    mat3<U64> b8;
    mat3<F32> b9;
    mat3<D64> b10;

    CHECK_EQUAL(sizeof(b5), (32 * 3 * 3) / 8);

    CHECK_EQUAL(sizeof(b7), sizeof(vec3<I64>) * 3);

    CHECK_EQUAL(sizeof(b1),  sizeof(I8)  * 3 * 3);
    CHECK_EQUAL(sizeof(b2),  sizeof(U8)  * 3 * 3);
    CHECK_EQUAL(sizeof(b3),  sizeof(I16) * 3 * 3);
    CHECK_EQUAL(sizeof(b4),  sizeof(U16) * 3 * 3);
    CHECK_EQUAL(sizeof(b5),  sizeof(I32) * 3 * 3);
    CHECK_EQUAL(sizeof(b6),  sizeof(U32) * 3 * 3);
    CHECK_EQUAL(sizeof(b7),  sizeof(I64) * 3 * 3);
    CHECK_EQUAL(sizeof(b8),  sizeof(U64) * 3 * 3);
    CHECK_EQUAL(sizeof(b9),  sizeof(F32) * 3 * 3);
    CHECK_EQUAL(sizeof(b10), sizeof(D64) * 3 * 3);
    CHECK_EQUAL(sizeof(b10), sizeof(b9)  * 2);

    mat4<I8>  c1;
    mat4<U8>  c2;
    mat4<I16> c3;
    mat4<U16> c4;
    mat4<I32> c5;
    mat4<U32> c6;
    mat4<I64> c7;
    mat4<U64> c8;
    mat4<F32> c9;
    mat4<D64> c10;

    CHECK_EQUAL(sizeof(c5), (32 * 4 * 4) / 8);

    CHECK_EQUAL(sizeof(c7), sizeof(vec4<I64>) * 4);
    CHECK_EQUAL(sizeof(c9), sizeof(vec4<F32>) * 4);
    CHECK_EQUAL(sizeof(c3), sizeof(a3) * 4);

    CHECK_EQUAL(sizeof(c1),  sizeof(I8)  * 4 * 4);
    CHECK_EQUAL(sizeof(c2),  sizeof(U8)  * 4 * 4);
    CHECK_EQUAL(sizeof(c3),  sizeof(I16) * 4 * 4);
    CHECK_EQUAL(sizeof(c4),  sizeof(U16) * 4 * 4);
    CHECK_EQUAL(sizeof(c5),  sizeof(I32) * 4 * 4);
    CHECK_EQUAL(sizeof(c6),  sizeof(U32) * 4 * 4);
    CHECK_EQUAL(sizeof(c7),  sizeof(I64) * 4 * 4);
    CHECK_EQUAL(sizeof(c8),  sizeof(U64) * 4 * 4);
    CHECK_EQUAL(sizeof(c9),  sizeof(F32) * 4 * 4);
    CHECK_EQUAL(sizeof(c10), sizeof(D64) * 4 * 4);
    CHECK_EQUAL(sizeof(b10), sizeof(b9)  * 2);
}

TEST(matNUnionTests)
{
    mat2<I32> input1;
    mat3<U8> input2;
    mat4<I16> input3;
    mat4<F32> input4;

    // Random, unique values
    input1.set(-1, 0, 1, 2);
    input2.set(1, 2, 3, 4, 5, 6, 7, 8, 9);
    input3.set(-8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8);
    for (U8 i = 0; i < 16; ++i) {
        input4.mat[i] = 22.345f * (i + 1);
    }

    // Quick constructor check
    CHECK_EQUAL(input2.element(1, 2), 6);
    // Check that element is just a direct access to m-member
    CHECK_EQUAL(input1.element(1, 2), input1.m[1][2]);
    CHECK_EQUAL(input2.element(2, 1), input2.m[2][1]);
    CHECK_EQUAL(input3.element(3, 2), input3.m[3][2]);
    CHECK_EQUAL(input4.element(2, 2), input4.m[2][2]);

    U8 row, column;
    U8 elementsPerLine = 2;
    for (row = 0; row < elementsPerLine; ++row) {
        for (column = 0; column < elementsPerLine; ++column) {
            CHECK_EQUAL(input1.element(row, column), input1.mat[row * elementsPerLine + column]);
        }
    }
    elementsPerLine = 3;
    for (row = 0; row < elementsPerLine; ++row) {
        for (column = 0; column < elementsPerLine; ++column) {
            CHECK_EQUAL(input2.element(row, column), input2.mat[row * elementsPerLine + column]);
        }
    }
    elementsPerLine = 4;
    for (row = 0; row < elementsPerLine; ++row) {
        for (column = 0; column < elementsPerLine; ++column) {
            CHECK_EQUAL(input3.element(row, column), input3.mat[row * elementsPerLine + column]);
        }
    }

    for (row = 0; row < elementsPerLine; ++row) {
        for (column = 0; column < elementsPerLine; ++column) {
            CHECK_EQUAL(input4.element(row, column), input4.mat[row * elementsPerLine + column]);
        }
    }
}

TEST_CONST_MEMBER_FUNCTION(matN, equalityOperator, NA)
{
    mat2<I32> input1[2];
    mat3<U8>  input2[2];
    mat4<I16> input3[2];
    mat4<F32> input4[2];

    // Random, unique values
    input1[0].set(-1, 0, 1, 2);
    input1[1].set(-1, 0, 1, 2);
    CHECK_TRUE(input1[0] == input1[1]);
    CHECK_FALSE(input1[0] != input1[1]);
    input1[1].mat[2] = 2;
    CHECK_TRUE(input1[0] != input1[1]);
    CHECK_FALSE(input1[0] == input1[1]);

    input2[0].set(1, 2, 3, 4, 5, 6, 7, 8, 9);
    input2[1].set(1, 2, 3, 4, 5, 6, 7, 8, 9);

    CHECK_TRUE(input2[0] == input2[1]);
    CHECK_FALSE(input2[0] != input2[1]);
    input2[1].m[2][0] = 22;
    CHECK_TRUE(input2[0] != input2[1]);
    CHECK_FALSE(input2[0] == input2[1]);

    input3[0].set(-8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8);
    input3[1].set(-8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8);
    CHECK_TRUE(input3[0] == input3[1]);
    CHECK_FALSE(input3[0] != input3[1]);
    input3[1].m[1][3] = 22;
    CHECK_TRUE(input3[0] != input3[1]);
    CHECK_FALSE(input3[0] == input3[1]);

    for (U8 i = 0; i < 2; ++i) {
        for (U8 j = 0; j < 16; ++j) {
            input4[i].mat[j] = 22.345f * j * (j % 2 == 0 ? -1.0f : 1.0f);
        }
    }
    CHECK_TRUE(input4[0] == input4[1]);
    CHECK_FALSE(input4[0] != input4[1]);
    
    for (U8 i = 0; i < 2; ++i) {
        for (U8 j = 0; j < 16; ++j) {
            input4[i].mat[j] = 22.345f * (j + i + 1);
        }
    }

    CHECK_TRUE(input4[0] != input4[1]);
    CHECK_FALSE(input4[0] == input4[1]);
}

TEST_MEMBER_FUNCTION(matN, identity, NA)
{
    mat2<F32> input1; input1.zero();
    const mat2<F32> result1(1.0f, 0.0f,
                            0.0f, 1.0f);
    CHECK_NOT_EQUAL(result1, input1);
    input1.identity();
    CHECK_TRUE(result1 == input1);

    mat3<F32> input2; input2.zero();
    const mat3<F32> result2(1.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 1.0f);

    CHECK_NOT_EQUAL(result2, input2);
    input2.identity();
    CHECK_EQUAL(result2, input2);

    mat4<F32> input3; input3.zero();
    const mat4<F32> result3(1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f);
    CHECK_NOT_EQUAL(result3, input3);
    input3.identity();
    CHECK_EQUAL(result3, input3);
}

TEST_MEMBER_FUNCTION(matN, transpose, NA)
{
    mat2<F32> input1(1.0f, 2.0f, 3.0f, 4.0f);
    const mat2<F32> result1(1.0f, 3.0f, 2.0f, 4.0f);
    const mat2<F32> result2(1.0f, 2.0f, 3.0f, 4.0f);
    CHECK_TRUE(input1.getTranspose() == result1);
    CHECK_TRUE(input1 != result1);
    input1.transpose();
    CHECK_TRUE(input1 == result1);
    input1.transpose();
    CHECK_TRUE(input1 == result2);


    mat3<I32> input2(1, 2, 3, 4, 5, 6, 7, 8, 9);
    const mat3<I32> result3(1, 4, 7, 2, 5, 8, 3, 6, 9);
    const mat3<I32> result4(1, 2, 3, 4, 5, 6, 7, 8, 9);
    CHECK_TRUE(input2.getTranspose() == result3);
    CHECK_TRUE(input2 != result3);
    input2.transpose();
    CHECK_TRUE(input2 == result3);
    input2.transpose();
    CHECK_TRUE(input2 == result4);


    mat4<F32> input3(1.0f, 2.0f, 3.0f, 4.0f,
                     5.0f, 6.0f, 7.0f, 8.0f,
                     9.0f, 10.0f, 11.0f, 12.0f,
                     13.0f, 14.0f, 15.0f, 16.0f);
    const mat4<F32> result5(1.0f, 5.0f, 9.0f, 13.0f,
                            2.0f, 6.0f, 10.0f, 14.0f,
                            3.0f, 7.0f, 11.0f, 15.0f,
                            4.0f, 8.0f, 12.0f, 16.0f);
    const mat4<F32> result6(1.0f, 2.0f, 3.0f, 4.0f,
                            5.0f, 6.0f, 7.0f, 8.0f,
                            9.0f, 10.0f, 11.0f, 12.0f,
                            13.0f, 14.0f, 15.0f, 16.0f);
    CHECK_TRUE(input3.getTranspose() == result5);
    CHECK_TRUE(input3 != result5);
    input3.transpose();
    CHECK_TRUE(input3 == result5);
    input3.transpose();
    CHECK_TRUE(input3 == result6);
}

TEST_MEMBER_FUNCTION(matN, multiplyOperator, scalar)
{
    const mat2<I32> input1(-2, -1,
                            1,  2);

    const mat2<I32> result1(-4, -2,
                             2,  4);

    CHECK_FALSE(input1 == result1);
    CHECK_TRUE((input1 * 2) == result1);


    const mat3<F32> input2(-5.0f, -4.0f, -3.0f, 
                           -2.0f, -1.0f,  1.0f,
                            2.0f,  3.0f,  4.0f);
    const mat3<F32> result2( 2.5f,  2.0f,  1.5f,
                             1.0f,  0.5f, -0.5f,
                            -1.0f, -1.5f, -2.0f);

    CHECK_FALSE(input2 == result2);
    CHECK_TRUE((input2 * (-0.5f)) == result2);


    const mat4<I32> input3(1, 2, 3, 4,
                           5, 6, 7, 8,
                           9, 8, 7, 6,
                           5, 4, 3, 2);
    const mat4<I32> result3( 3,  6,  9, 12,
                            15, 18, 21, 24,
                            27, 24, 21, 18,
                            15, 12,  9,  6);

    CHECK_FALSE(input3 == result3);
    CHECK_TRUE((input3 * 3) == result3);

    const mat4<F32> input4(1.0f, 2.0f, 3.0f, 4.0f,
                           5.0f, 6.0f, 7.0f, 8.0f,
                           9.0f, 8.0f, 7.0f, 6.0f,
                           5.0f, 4.0f, 3.0f, 2.0f);
    const mat4<F32> result4( 3.0f,  6.0f,  9.0f, 12.0f,
                            15.0f, 18.0f, 21.0f, 24.0f,
                            27.0f, 24.0f, 21.0f, 18.0f,
                            15.0f, 12.0f,  9.0f,  6.0f);

    CHECK_FALSE(input4 == result4);
    CHECK_TRUE((input4 * 3) == result4);
}

// this depends on multiply tests!
TEST_MEMBER_FUNCTION(matN, divideOperator, scalar)
{
    const mat2<I32> input1(-2, -2,
                            2,  2);
    CHECK_FALSE((input1 / 2) == (input1 * 2));
    CHECK_TRUE((input1 / 2) == (input1 * 0.5f));

    const mat3<F32> input2(-5.0f, -4.0f, -3.0f,
                           -2.0f, -1.0f, 1.0f,
                            2.0f, 3.0f, 4.0f);
    CHECK_FALSE((input2 / 2) == (input2 * 2));
    CHECK_TRUE((input2 / 2) == (input2 * 0.5f));

    const mat4<I32> input3(1, 2, 3, 4,
                           5, 6, 7, 8,
                           9, 8, 7, 6,
                           5, 4, 3, 2);
    CHECK_FALSE((input3 / 2) == (input3 * 2));

    CHECK_TRUE((input3 / 2) == (input3 * 0.5f));

    const mat4<F32> input4(1.0f, 2.0f, 3.0f, 4.0f,
                           5.0f, 6.0f, 7.0f, 8.0f,
                           9.0f, 8.0f, 7.0f, 6.0f,
                           5.0f, 4.0f, 3.0f, 2.0f);
    CHECK_FALSE((input4 / 2) == (input4 * 2));
    CHECK_TRUE((input4 / 2) == (input4 * 0.5f));
}

TEST_MEMBER_FUNCTION(matN, multiplyOperator, matN)
{
}

TEST_MEMBER_FUNCTION(matN, inverse, NA)
{

}

}; //namespace Divide