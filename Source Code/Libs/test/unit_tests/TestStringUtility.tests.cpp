/*
Copyright 2016 Colin Girling

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "../Test.hpp"
#include "../TestStringUtility.hpp"
#include <cstddef>
#include <limits.h>

#include "EnabledTests.h"

#if defined(TESTSTRINGUTILITY_TESTS_ENABLED) && (TESTSTRINGUTILITY_TESTS_ENABLED != 0)

TEST_MEMBER_FUNCTION(TestStringUtility, GetMinSignedIntAsString, unsigned_int)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("unsigned int");

    char const* str = TestStringUtility::GetMinSignedIntAsString(1U);
    CHECK_STRCMP(str, "-128");

    str = TestStringUtility::GetMinSignedIntAsString(2U);
    CHECK_STRCMP(str, "-32768");

    str = TestStringUtility::GetMinSignedIntAsString(4U);
    CHECK_STRCMP(str, "-2147483648");

    str = TestStringUtility::GetMinSignedIntAsString(8U);
    CHECK_STRCMP(str, "-9223372036854775808");
}

TEST_MEMBER_FUNCTION(TestStringUtility, GetMaxSignedIntAsString, unsigned_int)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("unsigned int");

    char const* str = TestStringUtility::GetMaxSignedIntAsString(1U);
    CHECK_STRCMP(str, "127");

    str = TestStringUtility::GetMaxSignedIntAsString(2U);
    CHECK_STRCMP(str, "32767");

    str = TestStringUtility::GetMaxSignedIntAsString(4U);
    CHECK_STRCMP(str, "2147483647");

    str = TestStringUtility::GetMaxSignedIntAsString(8U);
    CHECK_STRCMP(str, "9223372036854775807");
}

TEST_MEMBER_FUNCTION(TestStringUtility, GetMinSignedIntCharCount, unsigned_int)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("unsigned int");

    CHECK_EQUAL(TestStringUtility::GetMinSignedIntCharCount(1U), 4U);
    CHECK_EQUAL(TestStringUtility::GetMinSignedIntCharCount(2U), 6U);
    CHECK_EQUAL(TestStringUtility::GetMinSignedIntCharCount(4U), 11U);
    CHECK_EQUAL(TestStringUtility::GetMinSignedIntCharCount(8U), 20U);
}

TEST_MEMBER_FUNCTION(TestStringUtility, GetMaxSignedIntCharCount, unsigned_int)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("unsigned int");

    CHECK_EQUAL(TestStringUtility::GetMaxSignedIntCharCount(1U), 3U);
    CHECK_EQUAL(TestStringUtility::GetMaxSignedIntCharCount(2U), 5U);
    CHECK_EQUAL(TestStringUtility::GetMaxSignedIntCharCount(4U), 10U);
    CHECK_EQUAL(TestStringUtility::GetMaxSignedIntCharCount(8U), 19U);
}

TEST_MEMBER_FUNCTION(TestStringUtility, GetMaxUnsignedIntCharCount, unsigned_int)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("unsigned int");

    CHECK_EQUAL(TestStringUtility::GetMaxUnsignedIntCharCount(1U), 3U);
    CHECK_EQUAL(TestStringUtility::GetMaxUnsignedIntCharCount(2U), 5U);
    CHECK_EQUAL(TestStringUtility::GetMaxUnsignedIntCharCount(4U), 10U);
    CHECK_EQUAL(TestStringUtility::GetMaxUnsignedIntCharCount(8U), 20U);
}

TEST_MEMBER_FUNCTION(TestStringUtility, GetMaxUnsignedIntAsString, unsigned_int)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("unsigned char");

    char const* str = TestStringUtility::GetMaxUnsignedIntAsString(1U);
    CHECK_STRCMP(str, "255");

    str = TestStringUtility::GetMaxUnsignedIntAsString(2U);
    CHECK_STRCMP(str, "65535");

    str = TestStringUtility::GetMaxUnsignedIntAsString(4U);
    CHECK_STRCMP(str, "4294967295");

    str = TestStringUtility::GetMaxUnsignedIntAsString(8U);
    CHECK_STRCMP(str, "18446744073709551615");
}


TEST_MEMBER_FUNCTION(TestStringUtility, UnsafeFill, char_ptr_char_size_t)
{
    using ocl::TestStringUtility;

    char str[10];
    TestStringUtility::UnsafeFill(str, 'a', 1);
    CHECK_ZERO(MemCmp(str, "a", 1));

    TestStringUtility::UnsafeFill(str, 'b', 2);
    CHECK_ZERO(MemCmp(str, "bb", 2));

    TEST_OVERRIDE_ARGS("char*,char,size_t");

}

TEST_MEMBER_FUNCTION(TestStringUtility, SafeLength, char_const_ptr)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("char const*");

    CHECK_ZERO(TestStringUtility::SafeLength(static_cast<char const*>(NULL)));

    CHECK_EQUAL(TestStringUtility::SafeLength("a"), 1U);
}

TEST_MEMBER_FUNCTION(TestStringUtility, UnsafeFind, char_const_ptr_char_size_t_size_t)
{
    using ocl::TestStringUtility;
    typedef TestStringUtility::size_type size_type;

    TEST_OVERRIDE_ARGS("char const*,char,size_t&,size_t");

    char str[] = "abcABDabcd";

    size_type pos = 1U;
    TestStringUtility::UnsafeFind(str, 'a', pos, 0U);
    CHECK_EQUAL(pos, 0U);

    pos = 0U;
    TestStringUtility::UnsafeFind(str, 'b', pos, 0U);
    CHECK_EQUAL(pos, 1U);

    pos = 1U;
    TestStringUtility::UnsafeFind(str, 'A', pos, 0U);
    CHECK_EQUAL(pos, 3U);

    TestStringUtility::UnsafeFind(str, 'a', pos, 1U);
    CHECK_EQUAL(pos, 6U);

    TestStringUtility::UnsafeFind(str, 'c', pos, 2U);
    CHECK_EQUAL(pos, 2U);

    TestStringUtility::UnsafeFind(str, 'c', pos, 3U);
    CHECK_EQUAL(pos, 8U);

    TestStringUtility::UnsafeFind(str, 'd', pos, 1U);
    CHECK_EQUAL(pos, 9U);
}

TEST_MEMBER_FUNCTION(TestStringUtility, Allocate, char_ptr_ref_size_t)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("char*&,size_t");

    char* ptr = NULL;
    TestStringUtility::Allocate(ptr, 10);
    CHECK_NOT_NULL(ptr);
    TestStringUtility::FastFree(ptr);
}

TEST_MEMBER_FUNCTION(TestStringUtility, SafeFree, char_ptr_ref)
{
    using ocl::TestStringUtility;

    TEST_OVERRIDE_ARGS("char*&");

    char* ptr = NULL;
    TestStringUtility::Allocate(ptr, 10);
    CHECK_NOT_NULL(ptr);
    TestStringUtility::SafeFree(ptr);
    CHECK_NULL(ptr);
}

TEST_MEMBER_FUNCTION(TestStringUtility, SafeAllocateCopy, char_ptr_ref_size_t_ref_char_const_ptr_size_t)
{
    using ocl::TestStringUtility;
    typedef TestStringUtility::size_type size_type;

    TEST_OVERRIDE_ARGS("char*&,size_t&,char const*,size_t");

    char const* src = "Hello";
    size_type src_len = StrLen(src);

    char* dest = NULL;
    size_type dest_len = 0;
    TestStringUtility::SafeAllocateCopy(dest, dest_len, src, src_len);
    CHECK_NOT_NULL(dest);
    CHECK_STRCMP(dest, src);
    CHECK_EQUAL(dest_len, src_len);
    TestStringUtility::FastFree(dest);

    // When copying less than the whole string, ensure the new string is terminated with '\0'.
    TestStringUtility::SafeAllocateCopy(dest, dest_len, src, 1);
    CHECK_NOT_NULL(dest);
    bool is_null_terminated = *(dest + 1) == '\0';
    CHECK_TRUE(is_null_terminated);
    if (is_null_terminated)
        CHECK_STRCMP(dest, "H");
    CHECK_EQUAL(dest_len, 1U);
    TestStringUtility::FastFree(dest);

    // Test fail conditions return a NULL pointer and zero length.
    TestStringUtility::SafeAllocateCopy(dest, dest_len, static_cast<char*>(NULL), 1U);
    CHECK_NULL(dest);
    CHECK_ZERO(dest_len);

    TestStringUtility::SafeAllocateCopy(dest, dest_len, src, 0U);
    CHECK_NULL(dest);
    CHECK_ZERO(dest_len);

    TestStringUtility::SafeAllocateCopy(dest, dest_len, static_cast<char*>(NULL), 0U);
    CHECK_NULL(dest);
    CHECK_ZERO(dest_len);
}

TEST_MEMBER_FUNCTION(TestStringUtility, SafeAllocateCopy, char_ptr_ref_size_t_ref_char_const_ptr)
{
    using ocl::TestStringUtility;
    typedef TestStringUtility::size_type size_type;

    TEST_OVERRIDE_ARGS("char*&,size_t&,char const*");

    char const* src = "Hello";
    size_type src_len = StrLen(src);

    char* dest = NULL;
    size_type dest_len = 0;
    TestStringUtility::SafeAllocateCopy(dest, dest_len, src);
    CHECK_NOT_NULL(dest);
    CHECK_STRCMP(dest, src);
    CHECK_EQUAL(dest_len, src_len);
    TestStringUtility::FastFree(dest);

    // Test fail conditions return a NULL pointer and zero length.
    TestStringUtility::SafeAllocateCopy(dest, dest_len, static_cast<char*>(NULL));
    CHECK_NULL(dest);
    CHECK_ZERO(dest_len);
}

TEST_MEMBER_FUNCTION(TestStringUtility, UnsafeAllocateCopy, char_ptr_ref_size_t_ref_char_const_ptr)
{
    using ocl::TestStringUtility;
    typedef TestStringUtility::size_type size_type;

    TEST_OVERRIDE_ARGS("char*&,size_t&,char const*");

    char const* src = "Hello";
    size_type src_len = StrLen(src);

    char* dest = NULL;
    size_type dest_len = 0;
    TestStringUtility::UnsafeAllocateCopy(dest, dest_len, src);
    CHECK_NOT_NULL(dest);
    CHECK_STRCMP(dest, src);
    CHECK_EQUAL(dest_len, src_len);
    TestStringUtility::FastFree(dest);
}

TEST_MEMBER_FUNCTION(TestStringUtility, SafeReallocCopy, char_ptr_ref_size_t_char_const_ptr)
{
    using ocl::TestStringUtility;
    typedef TestStringUtility::size_type size_type;

    TEST_OVERRIDE_ARGS("char*&,size_t&,char const*");

    char const* src = "Hello";
    size_type src_len = StrLen(src);

    char* dest = NULL;
    size_type dest_len = 0;
    TestStringUtility::UnsafeAllocateCopy(dest, dest_len, src);
    CHECK_NOT_NULL(dest);
    CHECK_STRCMP(dest, src);
    CHECK_EQUAL(dest_len, src_len);
    TestStringUtility::FastFree(dest);
}

TEST_MEMBER_FUNCTION(TestStringUtility, SafeReallocCopy, char_ptr_ref_size_t_ref_char_const_ptr_size_t)
{
    using ocl::TestStringUtility;
    typedef TestStringUtility::size_type size_type;

    TEST_OVERRIDE_ARGS("char*&,size_t&,char const*,size_t");

    char const* src = "Hello";
    char const* src2 = "Goodbye";
    size_type src2_len = StrLen(src2);

    char* dest = NULL;
    size_type dest_len = 0;
    TestStringUtility::UnsafeAllocateCopy(dest, dest_len, src);
    TestStringUtility::SafeReallocCopy(dest, dest_len, src2, src2_len);
    CHECK_NOT_NULL(dest);
    CHECK_EQUAL(dest_len, src2_len);
    CHECK_EQUAL(StrLen(dest), src2_len);
    CHECK_STRCMP(dest, src2);
    TestStringUtility::FastFree(dest);
}

TEST_MEMBER_FUNCTION(TestStringUtility, SafeReallocAppend, char_ptr_ref_size_t_ref_char_const_ptr_size_t_char_const_ptr_size_t)
{
    using ocl::TestStringUtility;
    typedef TestStringUtility::size_type size_type;

    TEST_OVERRIDE_ARGS("char*&,size_t&,char const*,size_t,char const*,size_t");

    char const* src = "Hello";
    size_type src_len = StrLen(src);
    char const* src2 = "Goodbye";
    size_type src2_len = StrLen(src2);

    char* dest = NULL;
    size_type dest_len = 0;
    TestStringUtility::UnsafeAllocateCopy(dest, dest_len, src);
    TestStringUtility::SafeReallocAppend(dest, dest_len, dest, dest_len, src2, src2_len);
    CHECK_NOT_NULL(dest);
    CHECK_EQUAL(dest_len, src_len + src2_len);
    CHECK_EQUAL(StrLen(dest), src_len + src2_len);
    CHECK_STRCMP(dest, "HelloGoodbye");
    TestStringUtility::FastFree(dest);
}

#endif // #if defined(TESTSTRINGUTILITY_TESTS_ENABLED) && (TESTSTRINGUTILITY_TESTS_ENABLED != 0)
