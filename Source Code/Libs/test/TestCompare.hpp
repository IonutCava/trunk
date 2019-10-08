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

#ifndef OCL_GUARD_TEST_TESTCOMPARE_HPP
#define OCL_GUARD_TEST_TESTCOMPARE_HPP

#include <cstring>
#include <wchar.h>
#include "TestTypeInfo.hpp"

namespace ocl
{

template<typename Type1, typename Type2>
struct TestCompare
{
    typedef Type1 type1;
    typedef Type2 type2;
    typedef typename TestTypeInfo<Type1>::in_type in_type1;
    typedef typename TestTypeInfo<Type2>::in_type in_type2;

    static int Compare(in_type1 value1, in_type2 value2)
    {
        if (value1 < value2)
            return -1;
        return (value1 > value2) ? 1 : 0;
    }
};

template<>
struct TestCompare<bool, bool>
{
    typedef bool type1;
    typedef bool type2;
    typedef type1 in_type1;
    typedef type2 in_type2;

    static int Compare(type1 value1, type2 value2)
    {
        if (value1)
            return value2 ? 0 : 1;
        return value2 ? -1 : 0;
    }
};

template<>
struct TestCompare<char const*, char const*>
{
    typedef char const* type1;
    typedef char const* type2;
    typedef type1 in_type1;
    typedef type2 in_type2;

    static int Compare(type1 value1, type2 value2)
    {
        return ::strcmp(value1, value2);
    }
};

template<>
struct TestCompare<wchar_t const*, wchar_t const*>
{
    typedef wchar_t const* type1;
    typedef wchar_t const* type2;
    typedef type1 in_type1;
    typedef type2 in_type2;

    static int Compare(type1 value1, type2 value2)
    {
        return ::wcscmp(value1, value2);
    }
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTCOMPARE_HPP
