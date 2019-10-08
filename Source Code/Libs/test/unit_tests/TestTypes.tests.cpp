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
#include "../TestTypes.hpp"

#include "EnabledTests.h"

#if defined(TESTTYPES_TESTS_ENABLED) && (TESTTYPES_TESTS_ENABLED != 0)

TEST(TestTypes_64_bit)
{
    CHECK_EQUAL(sizeof(ocl::ocl_int64), 8U);
    CHECK_EQUAL(sizeof(ocl::ocl_uint64), 8U);
}

#endif // #if defined(TESTTYPES_TESTS_ENABLED) && (TESTTYPES_TESTS_ENABLED != 0)
