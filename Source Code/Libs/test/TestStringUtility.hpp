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

#ifndef OCL_GUARD_TEST_TESTSTRINGUTILITY_HPP
#define OCL_GUARD_TEST_TESTSTRINGUTILITY_HPP

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "TestTypes.hpp"
#include "TestMemoryUtility.hpp"

namespace ocl
{

/*
    String utility class providing features such as:
        allocation, de-allocation, filling, copying, appending 

    Functions that don't do pointer checking start with Unsafe in the name.
*/

struct TestStringUtility
{
    // NOTE: size_type is not used as it causes
    // possible loss of data warnings for some compilers.
    typedef ocl_size_type size_type;

    static void EnableMemoryCounting(bool enabled)
    {
        TestMemoryUtility<char>::GetCounter().Enable(enabled);
    }

    template<typename IntType>
    static char const* GetMinSignedIntAsString(IntType size_of_int) throw()
    {
        switch (size_of_int)
        {
            case 1: return "-128";
            case 2: return "-32768";
            case 4: return "-2147483648";
            case 8: return "-9223372036854775808";
        }

        return "";
    }

    template<typename IntType>
    static char const* GetMaxSignedIntAsString(IntType size_of_int) throw()
    {
        switch (size_of_int)
        {
            case 1: return "127";
            case 2: return "32767";
            case 4: return "2147483647";
            case 8: return "9223372036854775807";
        }

        return "";
    }

    template<typename IntType>
    static char const* GetMaxUnsignedIntAsString(IntType size_of_int) throw()
    {
        switch (size_of_int)
        {
            case 1: return "255";
            case 2: return "65535";
            case 4: return "4294967295";
            case 8: return "18446744073709551615";
        }

        return "";
    }

    /// Get maximum characters required for a string buffer to
    /// store a converted minimum signed integer value for a size of the int.
    template<typename IntType>
    static size_type GetMinSignedIntCharCount(IntType size_of_int) throw()
    {
        switch (size_of_int)
        {
            case 1: return 4U;
            case 2: return 6U;
            case 4: return 11U;
            case 8: return 20U;
        }

        return 0U;
    }

    /// Get maximum characters required for a string buffer to
    /// store a converted maximum signed integer value for a size of the int.
    template<typename IntType>
    static size_type GetMaxSignedIntCharCount(IntType size_of_int) throw()
    {
        switch (size_of_int)
        {
            case 1: return 3U;
            case 2: return 5U;
            case 4: return 10U;
            case 8: return 19U;
        }

        return 0U;
    }

    /// Get maximum characters required for a string buffer to
    /// store a converted maximum unsigned integer value for a size of the int.
    template<typename IntType>
    static size_type GetMaxUnsignedIntCharCount(IntType size_of_int) throw()
    {
        switch (size_of_int)
        {
            case 1: return 3U;
            case 2: return 5U;
            case 4: return 10U;
            case 8: return 20U;
        }

        return 0U;
    }

    /// Get maximum characters required for a string buffer to
    /// store a converted signed or unsigned integer value for a size of the int.
    template<typename IntType>
    static size_type GetMaxIntCharCount(IntType size_of_int) throw()
    {
        // Currently for the integer types the minimum signed int will
        // produce the maximum number of characters required for a string buffer.
        return GetMinSignedIntCharCount(size_of_int);
    }

    static bool IsNullChar(char ch)
    {
        return ch == '\0';
    }

    static bool IsNullChar(wchar_t ch)
    {
        return ch == L'\0';
    }

    static void SetNullChar(char& ch)
    {
        ch = '\0';
    }

    static void SetNullChar(wchar_t& ch)
    {
        ch = L'\0';
    }

    template<typename CharType>
    static void UnsafeFill(CharType* str, CharType ch, size_type len)
    {
        char* end = str + len;
        for (; str < end; ++str)
            *str = ch;
        SetNullChar(*str);
    }

    template<typename CharType>
    static CharType const* StrEnd(CharType const* str)
    {
        while (!IsNullChar(*str))
            ++str;
        return str;
    }

    template<typename CharType>
    static size_type UnsafeLength(CharType const* str)
    {
        return static_cast<ocl_size_type>(StrEnd(str) - str);
    }

    template<typename CharType>
    static size_type SafeLength(CharType const* str)
    {
        size_type len = str != NULL ? UnsafeLength(str) : 0U;
        return len;
    }

    template<typename CharType>
    static CharType* UnsafeCopy(CharType* dest, CharType const* src)
    {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
        size_t copy_size = static_cast<size_t>((UnsafeLength(src) + 1) * sizeof(CharType));
        return static_cast<CharType*>(::memcpy(dest, src, copy_size));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }

    template<typename CharType>
    static int UnsafeCompare(CharType const* str1, CharType const* str2)
    {
        return ::memcmp(str1, str2, (UnsafeLength(str1) + 1) * sizeof(CharType));
    }

    template<typename CharType>
    static size_type UnsafeCharCount(CharType const* str, CharType char_to_find)
    {
        size_type count = 0;
        for (; !IsNullChar(*str); ++str)
            if (*str == char_to_find)
                ++count;
        return count;
    }

    template<typename CharType>
    static ocl_size_type CharCount(CharType const* str, CharType const* chars_to_find)
    {
        CharType const* char_to_find;
        ocl_size_type count = 0;
        for (; !IsNullChar(*str); ++str)
            for (char_to_find = chars_to_find; !IsNullChar(*char_to_find); ++char_to_find)
                if (*str == *char_to_find)
                    ++count;
        return count;
    }

    template<typename CharType>
    static bool UnsafeFind(CharType const* str, CharType ch, size_type& pos, size_type start = 0)
    {
        CharType const* pch = str + start;
        for (; (!IsNullChar(*pch)) && (*pch != ch); ++pch)
            ;
        if (!IsNullChar(ch) && IsNullChar(*pch))
            return false;
        pos = static_cast<size_type>(pch - str);
        return true;
    }

    template<typename CharType>
    static void Allocate(CharType*& dest, size_type dest_len)
    {
        dest = TestMemoryUtility<CharType, size_type>::Allocate(dest_len + 1);
    }

    template<typename CharType>
    static void FastFree(CharType* str)
    {
        TestMemoryUtility<CharType>::FastFree(str);
    }

    template<typename CharType>
    static void SafeFree(CharType*& str)
    {
        TestMemoryUtility<CharType>::SafeFree(str);
    }

    template<typename CharType>
    static void SafeAllocateCopy(CharType*& dest,
                                 size_type& dest_len,
                                 CharType const* src,
                                 size_type src_len)
    {
        if ((src != NULL) && (src_len > 0))
        {
            dest = TestMemoryUtility<CharType, size_type>::UnsafeAllocateCopy(src, src_len + 1);
            if (dest != NULL)
            {
                dest_len = src_len;

                // If src is longer than src_len, add the terminating '\0'.
                if (!IsNullChar(*(src + src_len)))
                    SetNullChar(*(dest + dest_len));
            }
            else
                dest_len = static_cast<size_type>(0);
        }
        else
        {
            dest = NULL;
            dest_len = static_cast<size_type>(0);
        }
    }

    template<typename CharType>
    static void SafeAllocateCopy(CharType*& dest,
                                 size_type& dest_len,
                                 CharType const* src)
    {
        if (src == NULL)
        {
            dest = NULL;
            dest_len = static_cast<size_type>(0);
        }
        else
            UnsafeAllocateCopy(dest, dest_len, src);
    }

    template<typename CharType>
    static void UnsafeAllocateCopy(CharType*& dest,
                                   size_type& dest_len,
                                   CharType const* src,
                                   size_type src_len)
    {
        dest = TestMemoryUtility<CharType, size_type>::UnsafeAllocateCopy(src, src_len + 1);
        dest_len = (dest != NULL) ? src_len : static_cast<size_type>(0);
    }

    template<typename CharType>
    static void UnsafeAllocateCopy(CharType*& dest, size_type& dest_len, CharType const* src)
    {
        UnsafeAllocateCopy(dest, dest_len, src, UnsafeLength(src));
    }

    template<typename CharType>
    static void SafeReallocCopy(CharType*& dest,
                                size_type& dest_len,
                                CharType const* src,
                                size_type src_len)
    {
        FastFree(dest);
        SafeAllocateCopy(dest, dest_len, src, src_len);
    }

    template<typename CharType>
    static void SafeReallocCopy(CharType*& dest,
                                size_type& dest_len,
                                CharType const* src)
    {
        if (src != NULL)
        {
            size_type len = UnsafeLength(src);
            if (len > static_cast<size_type>(0))
            {
                if (len > dest_len)
                {
                    FastFree(dest);
                    UnsafeAllocateCopy(dest, dest_len, src);
                }
                else
                {
                    ::memcpy(dest, src, static_cast<size_t>(dest_len + 1));
                    dest_len = len;
                }
            }
            else
            {
                SafeFree(dest);
                dest_len = static_cast<size_type>(0);
            }
        }
        else
        {
            SafeFree(dest);
            dest_len = static_cast<size_type>(0);
        }
    }

    /// Allocate enough space in dest to copy str1 and append str2.
    /// @note It is safe for dest and str1 or str2 to be the same string.
    /// Also str1 or str2 can be NULL.
    template<typename CharType>
    static void SafeAllocAppend(CharType*& dest,
                                size_type& dest_len,
                                CharType const* str1,
                                size_type str1_len,
                                CharType const* str2,
                                size_type str2_len)
    {
        size_type new_len = str1_len + str2_len;
        if (new_len > static_cast<size_type>(0))
        {
            char* new_dest = NULL;
            Allocate(new_dest, new_len);
            if (new_dest != NULL)
            {
                if ((str1 != NULL) && (str1_len > static_cast<size_type>(0)))
                    ::memcpy(new_dest, str1, static_cast<size_t>(str1_len));
                else
                {
                    new_len -= str1_len;
                    str1_len = static_cast<size_type>(0);
                }
                if ((str2 != NULL) && (str2_len > static_cast<size_type>(0)))
                    ::memcpy(new_dest + str1_len, str2, static_cast<size_t>(str2_len));
                else
                    new_len -= str2_len;
                SetNullChar(*(new_dest + new_len));
                dest_len = new_len;
            }
            else
                dest_len = static_cast<size_type>(0);
            FastFree(dest);
            dest = new_dest;
        }
        else
        {
            SafeFree(dest);
            dest_len = static_cast<size_type>(0);
        }
    }

    /// Same as AllocAppend, except dest is first freed
    /// before being set with str1 and str2.
    template<typename CharType>
    static void SafeReallocAppend(CharType*& dest,
                                  size_type& dest_len,
                                  CharType const* str1,
                                  size_type str1_len,
                                  CharType const* str2,
                                  size_type str2_len)
    {
        CharType* new_dest = NULL;
        size_type new_dest_len = 0;
        SafeAllocAppend(new_dest, new_dest_len, str1, str1_len, str2, str2_len);
        FastFree(dest);
        dest = new_dest;
        dest_len = new_dest_len;
    }
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTSTRINGUTILITY_HPP
