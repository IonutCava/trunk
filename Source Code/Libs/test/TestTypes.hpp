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

#ifndef OCL_GUARD_TEST_TESTTYPES_HPP
#define OCL_GUARD_TEST_TESTTYPES_HPP

#if !defined(_MSC_VER)
// Want to use uint64_t for all non-Winsows platforms
#include <stdint.h>
#else
#include <inttypes.h>
#endif

namespace ocl
{
#if defined(_MSC_VER)

    #if _MSC_VER < 1600
        // For simplicity, always use Microsoft in-built 64-bit type,
        // as various Microsoft versions have different support.
        typedef signed __int64   ocl_int64;
        typedef unsigned __int64 ocl_uint64;
    #else
        typedef signed long long   ocl_int64;
        typedef unsigned long long ocl_uint64;
    #endif

    typedef signed char ocl_int8;
    typedef unsigned char ocl_uint8;
    typedef signed short ocl_int16;
    typedef unsigned short ocl_uint16;
    typedef signed int ocl_int32;
    typedef unsigned int ocl_uint32;

#else

    typedef int8_t ocl_int8;
    typedef uint8_t ocl_uint8;
    typedef int16_t ocl_int16;
    typedef uint16_t ocl_uint16;
    typedef int32_t ocl_int32;
    typedef uint32_t ocl_uint32;
    typedef int64_t  ocl_int64;
    typedef uint64_t ocl_uint64;

#endif

    typedef ocl_uint64 ocl_size_type;
}

#endif // OCL_GUARD_TEST_TESTTYPES_HPP
