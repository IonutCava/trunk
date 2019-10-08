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

******************************************************************************

Tests performed on AMD FX-8350 8 Core CPU with 32GB RAM in debug for VS2005.
*/

#include "../Test.hpp"
#include "../TestStringUtility.hpp"
#include "../TestMemoryUtility.hpp"

namespace
{

// Need to wrap call to TestStringUtility::GetString so the memory can be freed.
void TimeGetString(unsigned long value)
{
    unsigned int length = 0;
    char* str = ocl::TestStringUtility::GetString(value, length);
    ocl::TestMemoryUtility<char>::FastFree(str);
}

}

TEST_MEMBER_FUNCTION_TIME(TestStringUtility, GetString, unsigned_long_size_type, 0, 100)
{
#if defined(_DEBUG)
    // In trials all timings were between 96000 and 100000
    static unsigned long const min_expected_iterations = 85000;
#else
    // In trials all timings were between 402000 and 414000
    static unsigned long const min_expected_iterations = 390000;
#endif

    TEST_OVERRIDE_ARGS("unsigned long,size_type");

    unsigned int length = 0;
    unsigned long value = 0;

    CHECK_PERFORMANCE(TimeGetString(value++), min_expected_iterations);
}
