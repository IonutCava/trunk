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
#include "../TestConverterUtility.hpp"
#include "../TestMemoryUtility.hpp"
#include "../TestStringUtility.hpp"
#include "../TestMinMax.hpp"

#include "EnabledTests.h"

#if defined(TESTCONVERTERUTILITY_TESTS_ENABLED) && (TESTCONVERTERUTILITY_TESTS_ENABLED != 0)

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, bool_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    typedef unsigned int size_type;
    typedef bool type;

    TEST_OVERRIDE_ARGS("bool,size_type");

    type value = true;
    size_type length = 0;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "true");
    CHECK_EQUAL(length, 4U);
    TestMemoryUtility<char>::SafeFree(str);

    value = false;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "false");
    CHECK_EQUAL(length, 5U);
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, char_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    typedef unsigned int size_type;
    typedef char type;

    TEST_OVERRIDE_ARGS("char,size_type");

    type value = '1';
    size_type length = 0;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "1");
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, signed_char_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    using ocl::TestStringUtility;
    using ocl::TestMinMax;
    typedef unsigned int size_type;
    typedef signed char type;

    TEST_OVERRIDE_ARGS("signed char,size_type");

    type value = 0;
    size_type length = 0;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "0");
    TestMemoryUtility<char>::SafeFree(str);

    char const* min_str = TestStringUtility::GetMinSignedIntAsString(sizeof(type));
    value = TestMinMax<type>::min_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, min_str);
    TestMemoryUtility<char>::SafeFree(str);

    char const* max_str = TestStringUtility::GetMaxSignedIntAsString(sizeof(type));
    value = TestMinMax<type>::max_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, max_str);
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, unsigned_char_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    using ocl::TestStringUtility;
    using ocl::TestMinMax;
    typedef unsigned int size_type;
    typedef unsigned char type;

    TEST_OVERRIDE_ARGS("unsigned char,size_type");

    type value = 0;
    size_type length = 0U;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "0");
    TestMemoryUtility<char>::SafeFree(str);

    char const* max_str = TestStringUtility::GetMaxUnsignedIntAsString(sizeof(type));
    value = TestMinMax<type>::max_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, max_str);
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, signed_short_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    using ocl::TestStringUtility;
    using ocl::TestMinMax;
    typedef unsigned int size_type;
    typedef signed short type;

    TEST_OVERRIDE_ARGS("signed short,size_type");

    type value = 0;
    size_type length = 0;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "0");
    TestMemoryUtility<char>::SafeFree(str);

    char const* min_str = TestStringUtility::GetMinSignedIntAsString(sizeof(type));
    value = TestMinMax<type>::min_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, min_str);
    TestMemoryUtility<char>::SafeFree(str);

    char const* max_str = TestStringUtility::GetMaxSignedIntAsString(sizeof(type));
    value = TestMinMax<type>::max_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, max_str);
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, unsigned_short_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    using ocl::TestStringUtility;
    using ocl::TestMinMax;
    typedef unsigned int size_type;
    typedef unsigned short type;

    TEST_OVERRIDE_ARGS("unsigned short,size_type");

    type value = 0;
    size_type length = 0U;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "0");
    TestMemoryUtility<char>::SafeFree(str);

    char const* max_str = TestStringUtility::GetMaxUnsignedIntAsString(sizeof(type));
    value = TestMinMax<type>::max_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, max_str);
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, signed_int_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    using ocl::TestStringUtility;
    using ocl::TestMinMax;
    typedef unsigned int size_type;
    typedef signed int type;

    TEST_OVERRIDE_ARGS("signed int,size_type");

    type value = 0;
    size_type length = 0;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "0");
    TestMemoryUtility<char>::SafeFree(str);

    char const* min_str = TestStringUtility::GetMinSignedIntAsString(sizeof(type));
    value = TestMinMax<type>::min_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, min_str);
    TestMemoryUtility<char>::SafeFree(str);

    char const* max_str = TestStringUtility::GetMaxSignedIntAsString(sizeof(type));
    value = TestMinMax<type>::max_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, max_str);
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, unsigned_int_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    using ocl::TestStringUtility;
    using ocl::TestMinMax;
    typedef unsigned int size_type;
    typedef unsigned int type;

    TEST_OVERRIDE_ARGS("unsigned int,size_type");

    type value = 0;
    size_type length = 0U;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "0");
    TestMemoryUtility<char>::SafeFree(str);

    char const* max_str = TestStringUtility::GetMaxUnsignedIntAsString(sizeof(type));
    value = TestMinMax<type>::max_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, max_str);
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, signed_long_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    using ocl::TestStringUtility;
    using ocl::TestMinMax;
    typedef unsigned int size_type;
    typedef signed long type;

    TEST_OVERRIDE_ARGS("signed long,size_type");

    type value = 0;
    size_type length = 0;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "0");
    TestMemoryUtility<char>::SafeFree(str);

    char const* min_str = TestStringUtility::GetMinSignedIntAsString(sizeof(type));
    value = TestMinMax<type>::min_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, min_str);
    TestMemoryUtility<char>::SafeFree(str);

    char const* max_str = TestStringUtility::GetMaxSignedIntAsString(sizeof(type));
    value = TestMinMax<type>::max_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, max_str);
    TestMemoryUtility<char>::SafeFree(str);
}

TEST_MEMBER_FUNCTION(TestConverterUtility, GetString, unsigned_long_size_type)
{
    using ocl::TestConverterUtility;
    using ocl::TestMemoryUtility;
    using ocl::TestStringUtility;
    using ocl::TestMinMax;
    typedef unsigned int size_type;
    typedef unsigned long type;

    TEST_OVERRIDE_ARGS("unsigned long,size_type");

    type value = 0;
    size_type length = 0U;
    char* str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, "0");
    TestMemoryUtility<char>::SafeFree(str);

    char const* max_str = TestStringUtility::GetMaxUnsignedIntAsString(sizeof(type));
    value = TestMinMax<type>::max_value;
    length = 0;
    str = TestConverterUtility<type>::GetString(value, length);
    CHECK_NOT_NULL(str);
    CHECK_STRCMP(str, max_str);
    TestMemoryUtility<char>::SafeFree(str);
}

#endif // #if defined(TESTCONVERTERUTILITY_TESTS_ENABLED) && (TESTCONVERTERUTILITY_TESTS_ENABLED != 0)
