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

#ifndef OCL_GUARD_TEST_TESTDEBUGPRINT_HPP
#define OCL_GUARD_TEST_TESTDEBUGPRINT_HPP

#include "StdioTestLog.hpp"

namespace ocl
{

class TestDebugPrint
{
public:
    template<typename Type>
    static unsigned long CountDigits(Type value, unsigned long curr_count = 0)
    {
        if (value < static_cast<type>(0))
            return CountDigits<Type>(value * static_cast<Type>(-1), 1);
        if (value < static_cast<type>(10))
            return curr_count + 1U;
        return CountDigits(value / static_cast<Type>(10), curr_count + 1U);
    }

    static TestLog* GetDefaultLogger()
    {
        static StdioTestLog logger;
        return &logger;
    }

    // Print out debug information, including any ASCII characters in ptr.
    static void DebugPrintAscii(void* ptr, TestLog* logger)
    {
        if (logger == NULL)
            logger = GetDefaultLogger();

        if (ptr != NULL)
        {
            printf("\"");
            char const* char_ptr = (char const*)ptr;
            while ((*char_ptr >= 32) && (*char_ptr <= 127))
                printf("%c", *char_ptr++);
            printf("\"\n");
        }
        else
            printf("\n");
    }

    template<typename NumericType>
    static void DebugPrintInt(NumericType value, int pad, TestLog* logger)
    {
        if (logger == NULL)
            logger = GetDefaultLogger();

        char num_str[22];
        sprintf("%0*ld ", pad, static_cast<signed long>(value));
        if (msg != NULL)
            printf(msg);
        DebugPrint(ptr);
    }
};

}

#endif // OCL_GUARD_TEST_TESTDEBUGPRINT_HPP

