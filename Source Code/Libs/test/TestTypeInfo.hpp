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

#ifndef OCL_GUARD_TEST_TESTTYPEINFO_HPP
#define OCL_GUARD_TEST_TESTTYPEINFO_HPP

#include <cstring>
#include <wchar.h>

namespace ocl
{

template<typename Type>
struct TestTypeInfo
{
    typedef Type type;
    typedef Type const& in_type;
};

template<>
struct TestTypeInfo<bool>
{
    typedef bool type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<char>
{
    typedef char type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<wchar_t>
{
    typedef wchar_t type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<signed char>
{
    typedef signed char type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<unsigned char>
{
    typedef unsigned char type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<signed short>
{
    typedef signed short type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<unsigned short>
{
    typedef unsigned short type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<signed int>
{
    typedef signed int type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<unsigned int>
{
    typedef unsigned int type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<signed long>
{
    typedef signed long type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<unsigned long>
{
    typedef unsigned long type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<float>
{
    typedef float type;
    typedef type in_type;
};

template<>
struct TestTypeInfo<double>
{
    typedef double type;
    typedef type in_type;
};

template<typename Type>
struct TestTypeInfo<Type const*>
{
    typedef Type const* type;
    typedef type in_type;
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTTYPEINFO_HPP
