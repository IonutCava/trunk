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

#ifndef OCL_GUARD_TEST_TESTINTEGERUTILITY_HPP
#define OCL_GUARD_TEST_TESTINTEGERUTILITY_HPP

#include "TestTypes.hpp"

namespace ocl
{

struct TestIntegerUtility
{
    static bool IsDigit(char ch)
    {
        switch (ch)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return true;
        default:
            return false;
        }
    }

    static bool IsDigit(wchar_t ch)
    {
        switch (ch)
        {
        case L'0':
        case L'1':
        case L'2':
        case L'3':
        case L'4':
        case L'5':
        case L'6':
        case L'7':
        case L'8':
        case L'9':
            return true;
        default:
            return false;
        }
    }

    static bool IsMinus(char ch)
    {
        return ch == '-';
    }

    static bool IsMinus(wchar_t ch)
    {
        return ch == L'-';
    }

    // Return 1 or -1. If IntType is unsigned then 0 is returned.
    template<typename IntType>
    static IntType GetMinusSign(bool want_negative)
    {
        IntType multiply;
        if (want_negative)
        {
            multiply = static_cast<IntType>(-1);
            if (multiply >= 0)
                multiply = 0; // Error! IntType is unsigned.
        }
        else
            multiply = static_cast<IntType>(1);
        return multiply;
    }

    template<typename IntType>
    static IntType ToInt(char ch)
    {
        switch (ch)
        {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default:
            return 0;
        }
    }

    template<typename IntType>
    static IntType ToInt(wchar_t ch)
    {
        switch (ch)
        {
        case L'0': return 0;
        case L'1': return 1;
        case L'2': return 2;
        case L'3': return 3;
        case L'4': return 4;
        case L'5': return 5;
        case L'6': return 6;
        case L'7': return 7;
        case L'8': return 8;
        case L'9': return 9;
        default:
            return 0;
        }
    }

    template<typename CharType, typename IntType, typename SizeType>
    static bool ToInt(CharType const* str, IntType& value, SizeType& error_pos)
    {
        bool success = true;
        IntType multiply;
        if (str != NULL)
        {
            CharType const* start = str;
            bool is_minus = IsMinus(*str);
            multiply = GetMinusSign<IntType>(is_minus);
            if (multiply != 0)
            {
                if (is_minus)
                    ++str;
                CharType const* digit_start = str;
                IntType column = 1;
                if (IsDigit(*str))
                {
                    for (; IsDigit(*str); ++str)
                        ;
                    --str;
                    value = 0;
                    for (; str >= digit_start; --str)
                    {
                        value += ToInt<IntType>(*str) * column;
                        column *= 10;
                    }
                }
                else
                {
                    value = 0;
                    success = false;
                    error_pos = static_cast<SizeType>(str - start);
                }
            }
            else
            {
                success = false;
                error_pos = 0;
            }
        }
        else
        {
            value = 0;
            multiply = 0;
        }

        value *= multiply;
        return success;
    }

    template<typename CharType, typename IntType>
    static bool ToInt(CharType const* str, IntType& value)
    {
        ocl_size_type error_pos = 0;
        return ToInt<CharType, IntType, ocl_size_type>(str, value, error_pos);
    }

    template<typename CharType, typename IntType>
    static IntType ToInt(CharType const* str)
    {
        IntType value = 0;
        if (!ToInt(str, value))
            value = 0;
        return value;
    }
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTINTEGERUTILITY_HPP

