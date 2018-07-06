#include "Headers/Defines.h"
#include "Core/Math/Headers/MathMatrices.h"

namespace Divide {

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

}; //namespace Divide