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
#include "EnabledTests.h"

#if defined(TESTINTEGERUTILITY_TESTS_ENABLED) && (TESTINTEGERUTILITY_TESTS_ENABLED != 0)

TEST_MEMBER_FUNCTION(TestIntegerUtility, IsDigit, char)
{
    using ocl::TestIntegerUtility;

    CHECK_FALSE(TestIntegerUtility::IsDigit('/'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('0'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('1'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('2'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('3'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('4'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('5'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('6'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('7'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('8'));
    CHECK_TRUE(TestIntegerUtility::IsDigit('9'));
    CHECK_FALSE(TestIntegerUtility::IsDigit(':'));
}

TEST_MEMBER_FUNCTION(TestIntegerUtility, IsDigit, wchar_t)
{
    using ocl::TestIntegerUtility;

    CHECK_FALSE(TestIntegerUtility::IsDigit(L'/'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'0'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'1'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'2'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'3'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'4'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'5'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'6'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'7'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'8'));
    CHECK_TRUE(TestIntegerUtility::IsDigit(L'9'));
    CHECK_FALSE(TestIntegerUtility::IsDigit(L':'));
}

TEST_MEMBER_FUNCTION(TestIntegerUtility, IsMinus, char)
{
    using ocl::TestIntegerUtility;

    CHECK_FALSE(TestIntegerUtility::IsMinus('+'));
    CHECK_TRUE(TestIntegerUtility::IsMinus('-'));
}

TEST_MEMBER_FUNCTION(TestIntegerUtility, IsMinus, wchar_t)
{
    using ocl::TestIntegerUtility;

    CHECK_FALSE(TestIntegerUtility::IsMinus(L'+'));
    CHECK_TRUE(TestIntegerUtility::IsMinus(L'-'));
}

TEST_MEMBER_FUNCTION(TestIntegerUtility, GetMinusSign, bool)
{
    using ocl::TestIntegerUtility;

    CHECK_EQUAL(TestIntegerUtility::GetMinusSign<int>(true), -1);
    CHECK_EQUAL(TestIntegerUtility::GetMinusSign<int>(false), 1);
    CHECK_EQUAL(TestIntegerUtility::GetMinusSign<unsigned int>(true), 0U);
    CHECK_EQUAL(TestIntegerUtility::GetMinusSign<unsigned int>(false), 1U);
}

TEST_MEMBER_FUNCTION(TestIntegerUtility, ToInt, char)
{
    using ocl::TestIntegerUtility;

    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('/'), 0);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('0'), 0);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('1'), 1);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('2'), 2);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('3'), 3);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('4'), 4);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('5'), 5);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('6'), 6);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('7'), 7);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('8'), 8);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>('9'), 9);
    CHECK_EQUAL(TestIntegerUtility::ToInt<int>(':'), 0);
}

TEST_MEMBER_FUNCTION(TestIntegerUtility, ToInt, char_const_ptr_int_ref)
{
    using ocl::TestIntegerUtility;

    TEST_OVERRIDE_ARGS("char const, int&");

    char const* str = "0";

    int value = 1;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 0);

    str = "-0";
    value = 1;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 0);

    str = "1";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 1);

    str = "-1";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, -1);

    str = "12";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 12);

    str = "-12";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, -12);

    str = "21";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 21);

    str = "-21";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, -21);

    str = "123";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 123);

    str = "-123";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, -123);

    str = "a";
    value = 0;
    CHECK_FALSE(TestIntegerUtility::ToInt(str, value));

    str = "1 ";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));

    str = " 1";
    value = 0;
    CHECK_FALSE(TestIntegerUtility::ToInt(str, value));
}

TEST_MEMBER_FUNCTION(TestIntegerUtility, ToInt, wchar_t_const_ptr_int_ref)
{
    using ocl::TestIntegerUtility;

    TEST_OVERRIDE_ARGS("wchar_t const, int&");

    wchar_t const* str = L"0";

    int value = 1;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 0);

    str = L"-0";
    value = 1;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 0);

    str = L"1";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 1);

    str = L"-1";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, -1);

    str = L"12";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 12);

    str = L"-12";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, -12);

    str = L"21";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 21);

    str = L"-21";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, -21);

    str = L"123";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, 123);

    str = L"-123";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));
    CHECK_EQUAL(value, -123);

    str = L"a";
    value = 0;
    CHECK_FALSE(TestIntegerUtility::ToInt(str, value));

    str = L"1 ";
    value = 0;
    CHECK_TRUE(TestIntegerUtility::ToInt(str, value));

    str = L" 1";
    value = 0;
    CHECK_FALSE(TestIntegerUtility::ToInt(str, value));
}

#endif // defined(TESTINTEGERUTILITY_TESTS_ENABLED) && (TESTINTEGERUTILITY_TESTS_ENABLED != 0)
