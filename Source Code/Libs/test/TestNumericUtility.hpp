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

#ifndef OCL_GUARD_TEST_TESTNUMERICUTILITY_HPP
#define OCL_GUARD_TEST_TESTNUMERICUTILITY_HPP

#include "TestMemoryUtility.hpp"
#include "TestStringUtility.hpp"
#include "TestMinMax.hpp"
#include <cstddef>
#include <cstring>
#include <cstdio>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ocl
{

template<typename Type, typename SizeType = size_t>
struct TestNumericUtility
{
    static SizeType GetNumberOfCharsForInt(Type value)
    {
        SizeType char_count = 1;
        if (value < 0)
        {
            static Type const min_value = TestMinMax<Type>::min_value;

            // Make sure value *= -1 doesn't overflow.
            if (value == min_value)
                ++value;

            ++char_count;
            value *= static_cast<Type>(-1);
        }

        while (value > static_cast<Type>(9))
        {
            ++char_count;
            value /= static_cast<Type>(10);
        }

        return char_count;
    }

    /// Allocate a string big enough to store the maximum value
    /// with a '\0' terminating character.
    static char* GetString(Type value, const char* fmt, SizeType& length)
    {
        typedef TestStringUtility::size_type size_type;
        static SizeType const size_of_type = static_cast<SizeType>(sizeof(Type));

        // Get maximum number of characters required to convert Type into a string buffer.
        size_type max_chrs = TestStringUtility::GetMaxIntCharCount<Type>(size_of_type);
        SizeType max_chars = static_cast<SizeType>(max_chrs);

        // Convert the integer type into a string.
        char* str = NULL;
        if (max_chars > 0)
        {
            TestMemoryUtility<char, SizeType>::Allocate(str, max_chars + 1);
            if (str != NULL)
            {
                sprintf(str, fmt, value);
                length = static_cast<SizeType>(::strlen(str));
            }
        }
        return str;
    }
};

} // namespace ocl

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // OCL_GUARD_TEST_TESTNUMERICUTILITY_HPP
