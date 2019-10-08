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
#include "../TestMinMax.hpp"
#include <limits.h>

#include "EnabledTests.h"

#if defined(TESTMINMAX_TESTS_ENABLED) && (TESTMINMAX_TESTS_ENABLED != 0)

TEST(TestMinMax_class_constants)
{
    using ocl::TestMinMax;

    // Need to define constants before testing,
    // otherwise gcc produced undefined reference compiler error.

    {
        signed char const min_value = TestMinMax<signed char>::min_value;
        signed char const max_value = TestMinMax<signed char>::max_value;
        CHECK_EQUAL(min_value, CHAR_MIN);
        CHECK_EQUAL(max_value, CHAR_MAX);
    }

    {
        unsigned char const min_value = TestMinMax<unsigned char>::min_value;
        unsigned char const max_value = TestMinMax<unsigned char>::max_value;
        CHECK_EQUAL(min_value, 0U);
        CHECK_EQUAL(max_value, UCHAR_MAX);
    }

    {
        signed short min_value = TestMinMax<signed short>::min_value;
        signed short max_value = TestMinMax<signed short>::max_value;
        CHECK_EQUAL(min_value, SHRT_MIN);
        CHECK_EQUAL(max_value, SHRT_MAX);
    }

    {
        unsigned short min_value = TestMinMax<unsigned short>::min_value;
        unsigned short max_value = TestMinMax<unsigned short>::max_value;
        CHECK_EQUAL(min_value, 0U);
        CHECK_EQUAL(max_value, USHRT_MAX);
    }

    {
        signed int min_value = TestMinMax<signed int>::min_value;
        signed int max_value = TestMinMax<signed int>::max_value;
        CHECK_EQUAL(min_value, INT_MIN);
        CHECK_EQUAL(max_value, INT_MAX);
    }

    {
        unsigned int min_value = TestMinMax<unsigned int>::min_value;
        unsigned int max_value = TestMinMax<unsigned int>::max_value;
        CHECK_EQUAL(min_value, 0U);
        CHECK_EQUAL(max_value, UINT_MAX);
    }

    {
        signed long min_value = TestMinMax<signed long>::min_value;
        signed long max_value = TestMinMax<signed long>::max_value;
        CHECK_EQUAL(min_value, LONG_MIN);
        CHECK_EQUAL(max_value, LONG_MAX);
    }

    {
        unsigned long min_value = TestMinMax<unsigned long>::min_value;
        unsigned long max_value = TestMinMax<unsigned long>::max_value;
        CHECK_EQUAL(min_value, 0U);
        CHECK_EQUAL(max_value, ULONG_MAX);
    }
}

#endif // #if defined(TESTMINMAX_TESTS_ENABLED) && (TESTMINMAX_TESTS_ENABLED != 0)
