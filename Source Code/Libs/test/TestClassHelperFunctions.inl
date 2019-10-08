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

#ifndef OCL_TEST_HELPERS_DISABLED
protected:
    static size_type StrLen(char const* str)
    {
        return static_cast<size_type>(StrEnd(str) - str);
    }

    static size_type StrLen(wchar_t const* str)
    {
        return static_cast<size_type>(StrEnd(str) - str);
    }

    static char const* StrEnd(char const* str)
    {
        while (*str != '\0')
            ++str;
        return str;
    }

    static wchar_t const* StrEnd(wchar_t const* str)
    {
        while (*str != L'\0')
            ++str;
        return str;
    }

    static char* StrCpy(char* str1, char const* str2)
    {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
        return ::strcpy(str1, str2);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }

    static wchar_t* StrCpy(wchar_t* str1, wchar_t const* str2)
    {
        size_type const size = sizeof(wchar_t) * (StrLen(str2) + 1);
        return static_cast<wchar_t*>(::memcpy(str1, str2, size));
    }

    static int StrCmp(char const* str1, char const* str2)
    {
        return ::memcmp(str1, str2, StrLen(str1) + 1);
    }

    static int StrCmp(wchar_t const* str1, wchar_t const* str2)
    {
        return ::memcmp(str1, str2, sizeof(wchar_t) * (StrLen(str1) + 1));
    }

    static size_type CharCount(char const* str, char char_to_find)
    {
        size_type count = 0;
        for (; *str != '\0'; ++str)
            if (*str == char_to_find)
                ++count;
        return count;
    }

    static size_type CharCount(wchar_t const* str, wchar_t char_to_find)
    {
        size_type count = 0;
        for (; *str != L'\0'; ++str)
            if (*str == char_to_find)
                ++count;
        return count;
    }

    static size_type CharCount(char const* str, char const* chars_to_find)
    {
        char const* char_to_find;
        size_type count = 0;
        for (; *str != '\0'; ++str)
            for (char_to_find = chars_to_find; *char_to_find != '\0'; ++char_to_find)
                if (*str == *char_to_find)
                    ++count;
        return count;
    }

    static size_type CharCount(wchar_t const* str, wchar_t const* chars_to_find)
    {
        wchar_t const* char_to_find;
        size_type count = 0;
        for (; *str != L'\0'; ++str)
            for (char_to_find = chars_to_find; *char_to_find != L'\0'; ++char_to_find)
                if (*str == *char_to_find)
                    ++count;
        return count;
    }

    template<typename SizeType>
    static int MemCmp(void const* ptr1, void const* ptr2, SizeType size)
    {
        return ::memcmp(ptr1, ptr2, static_cast<size_type>(size));
    }

    template<typename Type, typename SizeType>
    static void MemSet(Type* ptr, SizeType count, Type value)
    {
        for (Type* ptr_end = ptr + count; ptr < ptr_end; ++ptr)
            *ptr = value;
    }

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

    template<typename CharType, typename IntType>
    static bool ToInt(CharType const* str, IntType& value, size_type& error_pos)
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
                    error_pos = static_cast<size_type>(str - start);
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
        size_type error_pos = 0;
        return ToInt<CharType, IntType>(str, value, error_pos);
    }

    template<typename CharType, typename IntType>
    static IntType ToInt(CharType const* str)
    {
        IntType value = 0;
        if (!ToInt(str, value))
            value = 0;
        return value;
    }

    static void Sleep(unsigned long milliseconds)
    {
        ocl::TestTime::Sleep(milliseconds);
    }
#endif // OCL_TEST_HELPERS_ENABLED
