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
#include "../TestCompare.hpp"
#include "EnabledTests.h"

#if defined(TESTCOMPARE_TESTS_ENABLED) && (TESTCOMPARE_TESTS_ENABLED != 0)

TEST_MEMBER_FUNCTION(TestCompare, Compare, bool_bool)
{
    using ocl::TestCompare;
    typedef TestCompare<bool, bool> compare_type;

    TEST_OVERRIDE_ARGS("bool,bool");

    CHECK_ZERO(compare_type::Compare(false, false));
    CHECK_ZERO(compare_type::Compare(true, true));
    CHECK_LESS(compare_type::Compare(false, true), 0);
    CHECK_GREATER(compare_type::Compare(true, false), 0);
}

TEST_MEMBER_FUNCTION(TestCompare, Compare, char_const_ptr_char_const_ptr)
{
    using ocl::TestCompare;
    typedef TestCompare<char const*, char const*> compare_type;

    TEST_OVERRIDE_ARGS("char const*,char const*");

    CHECK_ZERO(compare_type::Compare("a", "a"));
    CHECK_LESS(compare_type::Compare("a", "b"), 0);
    CHECK_GREATER(compare_type::Compare("b", "a"), 0);
}

TEST_MEMBER_FUNCTION(TestCompare, Compare, wchar_t_const_ptr_wchar_t_const_ptr)
{
    using ocl::TestCompare;
    typedef TestCompare<wchar_t const*, wchar_t const*> compare_type;

    TEST_OVERRIDE_ARGS("char const*,char const*");

    CHECK_ZERO(compare_type::Compare(L"a", L"a"));
    CHECK_LESS(compare_type::Compare(L"a", L"b"), 0);
    CHECK_GREATER(compare_type::Compare(L"b", L"a"), 0);
}

TEST_MEMBER_FUNCTION(TestCompare, Compare, signed_int_signed_int)
{
    using ocl::TestCompare;
    typedef TestCompare<signed int, signed int> compare_type;

    TEST_OVERRIDE_ARGS("signed int,signed int");

    CHECK_ZERO(compare_type::Compare(1, 1));
    CHECK_LESS(compare_type::Compare(-1, 0), 0);
    CHECK_LESS(compare_type::Compare(1, 2), 0);
    CHECK_GREATER(compare_type::Compare(0, -1), 0);
    CHECK_GREATER(compare_type::Compare(2, 1), 0);
}

TEST_MEMBER_FUNCTION(TestCompare, Compare, unsigned_int_unsigned_int)
{
    using ocl::TestCompare;
    typedef TestCompare<unsigned int, unsigned int> compare_type;

    TEST_OVERRIDE_ARGS("unsigned int,unsigned int");

    CHECK_ZERO(compare_type::Compare(1U, 1U));
    CHECK_LESS(compare_type::Compare(1U, 2U), 0);
    CHECK_GREATER(compare_type::Compare(2U, 1U), 0);
}

#endif // #if defined(TESTCOMPARE_TESTS_ENABLED) && (TESTCOMPARE_TESTS_ENABLED != 0)
