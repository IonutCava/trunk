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

#ifndef OCL_GUARD_TEST_TESTCONVERTERUTILITY_HPP
#define OCL_GUARD_TEST_TESTCONVERTERUTILITY_HPP

#include "TestMemoryUtility.hpp"
#include "TestNumericUtility.hpp"
#include "TestTypes.hpp"
#include <cstring>
#include <cstddef>

namespace ocl
{

/*
 * Helper class for converting primitive types to char* which can then be output.
 */

template<typename ValueType>
struct TestConverterUtility
{
    typedef ValueType type;

    template<typename SizeType>
    static char* GetString(type, SizeType& length) { length = 0; return NULL; }
};

template<>
struct TestConverterUtility<bool>
{
    typedef bool type;

    /// bool will be returned as a string of "true" or "false".
    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        char const* str = value ? "true" : "false";
        length = static_cast<SizeType>(::strlen(str));
        return TestMemoryUtility<char, SizeType>::UnsafeAllocateCopy(str, length + 1);
    }
};

template<>
struct TestConverterUtility<char>
{
    typedef char type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        char str[2] = {value, '\0'};
        length = 1;
        return TestMemoryUtility<char, SizeType>::UnsafeAllocateCopy(str, 2);
    }
};

template<>
struct TestConverterUtility<signed char>
{
    typedef signed char type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<signed int, SizeType>::GetString(value, "%d", length);
    }
};

template<>
struct TestConverterUtility<unsigned char>
{
    typedef unsigned short type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<unsigned int, SizeType>::GetString(value, "%u", length);
    }
};

template<>
struct TestConverterUtility<signed short>
{
    typedef signed short type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<signed int, SizeType>::GetString(value, "%d", length);
    }
};

template<>
struct TestConverterUtility<unsigned short>
{
    typedef unsigned short type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<unsigned int, SizeType>::GetString(value, "%u", length);
    }
};

template<>
struct TestConverterUtility<signed int>
{
    typedef signed int type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<type, SizeType>::GetString(value, "%d", length);
    }
};

template<>
struct TestConverterUtility<unsigned int>
{
    typedef unsigned int type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<type, SizeType>::GetString(value, "%u", length);
    }
};

template<>
struct TestConverterUtility<signed long>
{
    typedef signed long type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<type, SizeType>::GetString(value, "%ld", length);
    }
};

template<>
struct TestConverterUtility<unsigned long>
{
    typedef unsigned long type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<type, SizeType>::GetString(value, "%lu", length);
    }
};

// Due to problems with overloading long, long long and int64 types,
// handle Microsoft C++ differently from compilers supporting long long.
#ifdef _MSC_VER

template<>
struct TestConverterUtility<ocl_int64>
{
    typedef ocl_int64 type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<type, SizeType>::GetString(value, "%I64d", length);
    }
};

template<>
struct TestConverterUtility<ocl_uint64>
{
    typedef ocl_uint64 type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<type, SizeType>::GetString(value, "%I64u", length);
    }
};

#else

template<>
struct TestConverterUtility<signed long long>
{
    typedef signed long type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<type, SizeType>::GetString(value, "%lld", length);
    }
};

template<>
struct TestConverterUtility<unsigned long long>
{
    typedef unsigned long type;

    template<typename SizeType>
    static char* GetString(type value, SizeType& length)
    {
        return TestNumericUtility<type, SizeType>::GetString(value, "%llu", length);
    }
};

#endif // _MSC_VER

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTCONVERTERUTILITY_HPP
