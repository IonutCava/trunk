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

#if defined(TESTCLASS_TESTS_ENABLED) && (TESTCLASS_TESTS_ENABLED != 0)

TEST_MEMBER_FUNCTION(TestClass, StrLen, char_const_ptr)
{
    TEST_OVERRIDE_ARGS("char const*");

    CHECK_ZERO(StrLen(""));
    CHECK_EQUAL(StrLen("a"), 1U);
}

TEST_MEMBER_FUNCTION(TestClass, StrLen, wchar_t_const_ptr)
{
    TEST_OVERRIDE_ARGS("wchar_t const*");

    CHECK_ZERO(StrLen(L""));
    CHECK_EQUAL(StrLen(L"a"), 1U);
}

TEST_MEMBER_FUNCTION(TestClass, StrCmp, char_const_ptr_char_const_ptr)
{
    TEST_OVERRIDE_ARGS("char const*,char const*");

    CHECK_STRCMP("", "");
    CHECK_STRCMP("a", "a");

    CHECK_LESS(StrCmp("", "a"), 0);
    CHECK_LESS(StrCmp("a", "b"), 0);
    CHECK_LESS(StrCmp("a", "aa"), 0);

    CHECK_GREATER(StrCmp("aa", "a"), 0);
    CHECK_GREATER(StrCmp("b", "a"), 0);
    CHECK_GREATER(StrCmp("a", ""), 0);
}

TEST_MEMBER_FUNCTION(TestClass, StrCmp, wchar_t_const_ptr_wchar_t_const_ptr)
{
    TEST_OVERRIDE_ARGS("wchar_t const*,wchar_t const*");

    CHECK_STRCMP(L"", L"");
    CHECK_STRCMP(L"a", L"a");

    CHECK_LESS(StrCmp(L"", L"a"), 0);
    CHECK_LESS(StrCmp(L"a", L"b"), 0);
    CHECK_LESS(StrCmp(L"a", L"aa"), 0);

    CHECK_GREATER(StrCmp(L"aa", L"a"), 0);
    CHECK_GREATER(StrCmp(L"b", L"a"), 0);
    CHECK_GREATER(StrCmp(L"a", L""), 0);
}

TEST_MEMBER_FUNCTION(TestClass, StrEnd, char_const_ptr)
{
    TEST_OVERRIDE_ARGS("char const*");

    char const* str = "";
    CHECK_EQUAL(StrEnd(str), str);

    str = "a";
    CHECK_EQUAL(StrEnd(str), str + 1);
}

TEST_MEMBER_FUNCTION(TestClass, StrEnd, wchar_t_const_ptr)
{
    TEST_OVERRIDE_ARGS("wchar_t const*");

    wchar_t const* str = L"";
    CHECK_EQUAL(StrEnd(str), str);

    str = L"a";
    CHECK_EQUAL(StrEnd(str), str + 1);
}

TEST_MEMBER_FUNCTION(TestClass, StrCpy, char_ptr_char_const_ptr)
{
    TEST_OVERRIDE_ARGS("char*, char const*");

    char const* str_hello = "Hello";

    char str[6];
    StrCpy(str, str_hello);
    CHECK_STRCMP(str, str_hello);
}

TEST_MEMBER_FUNCTION(TestClass, StrCpy, wchar_t_ptr_wchar_t_const_ptr)
{
    TEST_OVERRIDE_ARGS("wchar_t*, wchar_t const*");

    wchar_t const* str_hello = L"Hello";

    wchar_t str[6];
    StrCpy(str, str_hello);
    CHECK_STRCMP(str, str_hello);
}

TEST_MEMBER_FUNCTION(TestClass, CharCount, char_const_ptr_char)
{
    TEST_OVERRIDE_ARGS("char const*, char");

    char const* str = "";
    CHECK_ZERO(CharCount(str, 'a'));

    str = "b";
    CHECK_ZERO(CharCount(str, 'a'));

    str = "a";
    CHECK_EQUAL(CharCount(str, 'a'), 1U);

    str = "aa";
    CHECK_EQUAL(CharCount(str, 'a'), 2U);

    str = "aba";
    CHECK_EQUAL(CharCount(str, 'a'), 2U);
}

TEST_MEMBER_FUNCTION(TestClass, CharCount, wchar_t_const_ptr_wchar_t)
{
    TEST_OVERRIDE_ARGS("wchar_t const*, wchar_t");

    wchar_t const* str = L"";
    CHECK_ZERO(CharCount(str, L'a'));

    str = L"b";
    CHECK_ZERO(CharCount(str, L'a'));

    str = L"a";
    CHECK_EQUAL(CharCount(str, L'a'), 1U);

    str = L"aa";
    CHECK_EQUAL(CharCount(str, L'a'), 2U);

    str = L"aba";
    CHECK_EQUAL(CharCount(str, L'a'), 2U);
}

TEST_MEMBER_FUNCTION(TestClass, MemCmp, void_const_ptr_void_const_ptr_size_t)
{
    TEST_OVERRIDE_ARGS("void const*, void const*, size_t");

    CHECK_ZERO(MemCmp("1", "1", 1));
    CHECK_LESS(MemCmp("0", "1", 1), 0);
    CHECK_GREATER(MemCmp("1", "0", 1), 0);
    CHECK_ZERO(MemCmp(L"1", L"1", sizeof(wchar_t)));
    CHECK_LESS(MemCmp(L"0", L"1", sizeof(wchar_t)), 0);
    CHECK_GREATER(MemCmp(L"1", L"0", sizeof(wchar_t)), 0);
}

TEST_MEMBER_FUNCTION(TestClass, MemSet, char_ptr_size_t_char)
{
    TEST_OVERRIDE_ARGS("char*, size_t, char");

    size_t const num_chars = 2;

    wchar_t str[num_chars];

    MemSet(str, num_chars, L'a');
    CHECK_ZERO(MemCmp(str, L"aa", num_chars));
}

TEST_MEMBER_FUNCTION(TestClass, MemSet, wchar_t_ptr_size_t_wchar_t)
{
    TEST_OVERRIDE_ARGS("wchar_t*, size_t, wchar_t");

    size_t const num_chars = 2;

    wchar_t str[num_chars];

    MemSet(str, num_chars, L'a');
    CHECK_ZERO(MemCmp(str, L"aa", num_chars));
}

TEST_MEMBER_FUNCTION(TestClass, IsDigit, char)
{
    CHECK_FALSE(IsDigit('/'));
    CHECK_TRUE(IsDigit('0'));
    CHECK_TRUE(IsDigit('1'));
    CHECK_TRUE(IsDigit('2'));
    CHECK_TRUE(IsDigit('3'));
    CHECK_TRUE(IsDigit('4'));
    CHECK_TRUE(IsDigit('5'));
    CHECK_TRUE(IsDigit('6'));
    CHECK_TRUE(IsDigit('7'));
    CHECK_TRUE(IsDigit('8'));
    CHECK_TRUE(IsDigit('9'));
    CHECK_FALSE(IsDigit(':'));
}

TEST_MEMBER_FUNCTION(TestClass, IsDigit, wchar_t)
{
    CHECK_FALSE(IsDigit(L'/'));
    CHECK_TRUE(IsDigit(L'0'));
    CHECK_TRUE(IsDigit(L'1'));
    CHECK_TRUE(IsDigit(L'2'));
    CHECK_TRUE(IsDigit(L'3'));
    CHECK_TRUE(IsDigit(L'4'));
    CHECK_TRUE(IsDigit(L'5'));
    CHECK_TRUE(IsDigit(L'6'));
    CHECK_TRUE(IsDigit(L'7'));
    CHECK_TRUE(IsDigit(L'8'));
    CHECK_TRUE(IsDigit(L'9'));
    CHECK_FALSE(IsDigit(L':'));
}

TEST_MEMBER_FUNCTION(TestClass, IsMinus, char)
{
    CHECK_FALSE(IsMinus('+'));
    CHECK_TRUE(IsMinus('-'));
}

TEST_MEMBER_FUNCTION(TestClass, IsMinus, wchar_t)
{
    CHECK_FALSE(IsMinus(L'+'));
    CHECK_TRUE(IsMinus(L'-'));
}

TEST_MEMBER_FUNCTION(TestClass, GetMinusSign, bool)
{
    CHECK_EQUAL(GetMinusSign<int>(true), -1);
    CHECK_EQUAL(GetMinusSign<int>(false), 1);
    CHECK_EQUAL(GetMinusSign<unsigned int>(true), 0U);
    CHECK_EQUAL(GetMinusSign<unsigned int>(false), 1U);
}

TEST_MEMBER_FUNCTION(TestClass, ToInt, char)
{
    CHECK_EQUAL(ToInt<int>('/'), 0);
    CHECK_EQUAL(ToInt<int>('0'), 0);
    CHECK_EQUAL(ToInt<int>('1'), 1);
    CHECK_EQUAL(ToInt<int>('2'), 2);
    CHECK_EQUAL(ToInt<int>('3'), 3);
    CHECK_EQUAL(ToInt<int>('4'), 4);
    CHECK_EQUAL(ToInt<int>('5'), 5);
    CHECK_EQUAL(ToInt<int>('6'), 6);
    CHECK_EQUAL(ToInt<int>('7'), 7);
    CHECK_EQUAL(ToInt<int>('8'), 8);
    CHECK_EQUAL(ToInt<int>('9'), 9);
    CHECK_EQUAL(ToInt<int>(':'), 0);
}

TEST_MEMBER_FUNCTION(TestClass, ToInt, char_const_ptr_int_ref)
{
    TEST_OVERRIDE_ARGS("char const*, int&");

    char const* str = "0";

    int value = 1;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 0);

    str = "-0";
    value = 1;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 0);

    str = "1";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 1);

    str = "-1";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, -1);

    str = "12";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 12);

    str = "-12";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, -12);

    str = "21";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 21);

    str = "-21";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, -21);

    str = "123";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 123);

    str = "-123";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, -123);

    str = "a";
    value = 0;
    CHECK_FALSE(ToInt(str, value));

    str = "1 ";
    value = 0;
    CHECK_TRUE(ToInt(str, value));

    str = " 1";
    value = 0;
    CHECK_FALSE(ToInt(str, value));
}

TEST_MEMBER_FUNCTION(TestClass, ToInt, wchar_t_const_ptr_int_ref)
{
    TEST_OVERRIDE_ARGS("wchar_t const, int&");

    wchar_t const* str = L"0";

    int value = 1;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 0);

    str = L"-0";
    value = 1;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 0);

    str = L"1";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 1);

    str = L"-1";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, -1);

    str = L"12";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 12);

    str = L"-12";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, -12);

    str = L"21";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 21);

    str = L"-21";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, -21);

    str = L"123";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, 123);

    str = L"-123";
    value = 0;
    CHECK_TRUE(ToInt(str, value));
    CHECK_EQUAL(value, -123);

    str = L"a";
    value = 0;
    CHECK_FALSE(ToInt(str, value));

    str = L"1 ";
    value = 0;
    CHECK_TRUE(ToInt(str, value));

    str = L" 1";
    value = 0;
    CHECK_FALSE(ToInt(str, value));
}

#endif // #if defined(TESTCLASS_TESTS_ENABLED) && (TESTCLASS_TESTS_ENABLED != 0)
