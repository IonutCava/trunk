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
#include "../TestMemoryUtility.hpp"

#include "EnabledTests.h"

#if defined(TESTMEMORYUTILITY_TESTS_ENABLED) && (TESTMEMORYUTILITY_TESTS_ENABLED != 0)

TEST_MEMBER_FUNCTION(TestMemoryUtility, Allocate, size_t)
{
    using ocl::TestMemoryUtility;

    char* ptr = TestMemoryUtility<char>::Allocate(100);

    CHECK_NOT_NULL(ptr);
    TestMemoryUtility<char>::FastFree(ptr);
}

TEST_MEMBER_FUNCTION(TestMemoryUtility, Allocate, char_ptr_ref_size_t)
{
    using ocl::TestMemoryUtility;

    TEST_OVERRIDE_ARGS("char*&, size_t");

    char* ptr = NULL;
    TestMemoryUtility<char>::Allocate(ptr, 100);

    CHECK_NOT_NULL(ptr);
    TestMemoryUtility<char>::FastFree(ptr);
}

TEST_MEMBER_FUNCTION(TestMemoryUtility, FastFree, char_ptr)
{
    using ocl::TestMemoryUtility;

    TEST_OVERRIDE_ARGS("char*");

    char* ptr = NULL;
    TestMemoryUtility<char>::Allocate(ptr, 100);

    CHECK_NOT_NULL(ptr);
    TestMemoryUtility<char>::FastFree(ptr);
    CHECK_NOT_NULL(ptr);
}

TEST_MEMBER_FUNCTION(TestMemoryUtility, SafeFree, char_ptr_ref)
{
    using ocl::TestMemoryUtility;

    TEST_OVERRIDE_ARGS("char*&");

    char* ptr = NULL;
    TestMemoryUtility<char>::Allocate(ptr, 100);

    CHECK_NOT_NULL(ptr);
    TestMemoryUtility<char>::SafeFree(ptr);
    CHECK_NULL(ptr);
}

TEST_MEMBER_FUNCTION(TestMemoryUtility, UnsafeAllocateCopy, char_const_ptr_size_t)
{
    using ocl::TestMemoryUtility;

    TEST_OVERRIDE_ARGS("char const*, size_t");

    char* ptr = TestMemoryUtility<char>::UnsafeAllocateCopy("Hello", 6);
    CHECK_NOT_NULL(ptr);
    CHECK_ZERO(MemCmp(ptr, "Hello", 6));

    TestMemoryUtility<char>::FastFree(ptr);
}

TEST_MEMBER_FUNCTION(TestMemoryUtility, UnsafeAllocateCopy, char_ptr_ref_char_const_ptr_size_t)
{
    using ocl::TestMemoryUtility;

    TEST_OVERRIDE_ARGS("char*&, char const*, size_t");

    char* ptr = NULL;
    TestMemoryUtility<char>::UnsafeAllocateCopy(ptr, "Hello", 6);
    CHECK_NOT_NULL(ptr);
    CHECK_ZERO(MemCmp(ptr, "Hello", 6));

    TestMemoryUtility<char>::FastFree(ptr);
}

#endif // #if defined(TESTMEMORYUTILITY_TESTS_ENABLED) && (TESTMEMORYUTILITY_TESTS_ENABLED != 0)
