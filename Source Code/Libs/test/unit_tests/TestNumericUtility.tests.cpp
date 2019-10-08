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
#include "../TestNumericUtility.hpp"
#include <limits.h>

#include "EnabledTests.h"

#if defined(TESTNUMERICUTILITY_TESTS_ENABLED) && (TESTNUMERICUTILITY_TESTS_ENABLED != 0)

TEST_MEMBER_FUNCTION(TestNumericUtility, GetNumberOfCharsForInt, signed_char)
{
    typedef ocl::TestNumericUtility<signed char> test_numeric_utility;

    TEST_OVERRIDE_ARGS("signed char");

    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(0), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(1), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(8), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(9), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(10), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(11), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(98), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(99), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(100), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(101), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(CHAR_MAX - 1), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(CHAR_MAX), 3U);

    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-1), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-8), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-9), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-10), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-11), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-99), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-100), 4U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-101), 4U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(CHAR_MIN + 1), 4U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(CHAR_MIN), 4U);
}

TEST_MEMBER_FUNCTION(TestNumericUtility, GetNumberOfCharsForInt, unsigned_char)
{
    typedef ocl::TestNumericUtility<unsigned char> test_numeric_utility;

    TEST_OVERRIDE_ARGS("unsigned char");

    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(1), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(8), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(9), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(10), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(11), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(98), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(99), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(100), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(101), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(UCHAR_MAX - 1), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(UCHAR_MAX), 3U);
}

TEST_MEMBER_FUNCTION(TestNumericUtility, GetNumberOfCharsForInt, signed_short)
{
    typedef ocl::TestNumericUtility<signed short> test_numeric_utility;

    TEST_OVERRIDE_ARGS("signed short");

    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(0), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(1), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(8), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(9), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(10), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(11), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(98), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(99), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(100), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(101), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(SHRT_MAX - 1), 5U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(SHRT_MAX), 5U);

    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-1), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-8), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-9), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-10), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-11), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-99), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-100), 4U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(-101), 4U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(SHRT_MIN + 1), 6U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(SHRT_MIN), 6U);
}

TEST_MEMBER_FUNCTION(TestNumericUtility, GetNumberOfCharsForInt, unsigned_short)
{
    typedef ocl::TestNumericUtility<unsigned short> test_numeric_utility;

    TEST_OVERRIDE_ARGS("unsigned short");

    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(1), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(8), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(9), 1U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(10), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(11), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(98), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(99), 2U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(100), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(101), 3U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(USHRT_MAX - 1), 5U);
    CHECK_EQUAL(test_numeric_utility::GetNumberOfCharsForInt(USHRT_MAX), 5U);
}

#endif // #if defined(TESTNUMERICUTILITY_TESTS_ENABLED) && (TESTNUMERICUTILITY_TESTS_ENABLED != 0)
