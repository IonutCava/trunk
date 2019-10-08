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

#ifndef OCL_GUARD_TEST_TESTSTRING_HPP
#define OCL_GUARD_TEST_TESTSTRING_HPP

#include "TestStringUtility.hpp"
#include "TestConverterUtility.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace ocl
{

/*
 * String class for test harness, with an interface complete enough for general usage.
 */

class TestString
{
public:
    typedef TestStringUtility::size_type size_type;

    /// size_type has the potential of being signed or unsigned,
    /// so ensure signed/unsigned mismatch compiler warnings are suppressed.
    static size_type const size_type_default = static_cast<size_type>(0);

public:
    TestString(char const* str = NULL)
        : m_length(size_type_default)
        , m_string(NULL)
    {
        if ((str != NULL) && (*str != '\0'))
            TestStringUtility::UnsafeAllocateCopy(m_string, m_length, str);
    }

    TestString(char ch, size_type len)
        : m_length(len)
        , m_string(NULL)
    {
        if (len > size_type_default)
        {
            TestStringUtility::Allocate(m_string, len);
            if (m_string != NULL)
                TestStringUtility::UnsafeFill(m_string, ch, len);
            else
                m_length = size_type_default;
        }
    }

    TestString(TestString const& str)
        : m_length(size_type_default)
        , m_string(NULL)
    {
        TestStringUtility::SafeAllocateCopy(m_string, m_length, str.m_string, str.m_length);
    }

    ~TestString()
    {
        TestStringUtility::FastFree(m_string);
    }

    TestString& operator =(char const* str)
    {
        if ((str != NULL) && (*str != '\0'))
            TestStringUtility::SafeReallocCopy(m_string, m_length, str);
        else
            Clear();
        return *this;
    }

    TestString& operator =(TestString const& str)
    {
        if ((str.m_string != NULL) &&
            (*str.m_string != '\0') &&
            (str.m_length > size_type_default))
        {
            TestStringUtility::SafeReallocCopy(m_string,
                                               m_length,
                                               str.m_string,
                                               str.m_length);
        }
        else
            Clear();

        return *this;
    }

    TestString& operator +=(char const* str)
    {
        Append(str);
        return *this;
    }

    TestString& operator +=(TestString const& str)
    {
        Append(str);
        return *this;
    }

    char operator [](size_type index) const
    {
        return ((m_string != NULL) && (index < m_length)) ? m_string[index] : '\0';
    }

    char& operator [](size_type index)
    {
        static char spare = '\0';
        return ((m_string != NULL) && (index < m_length)) ? m_string[index] : spare;
    }

    operator char const*() const throw()
    {
        return Ptr();
    }

    bool operator ==(char const* str) const
    {
        if (m_string == NULL)
            return (str == NULL) || (*str == '\0');
        if (str != NULL)
            return ::strcmp(m_string, str) == 0;
        return false;
    }

    bool operator ==(TestString const& str) const
    {
        if (m_string == NULL)
            return (str.m_string == NULL) || (*str.m_string == '\0');
        if (str.m_string != NULL)
            return ::strcmp(m_string, str.Ptr()) == 0;
        return false;
    }

    bool operator !=(char const* str) const
    {
        return !(*this == str);
    }

    bool operator !=(TestString const& str) const
    {
        return !(*this == str);
    }

public:
    char const* Ptr() const throw()
    {
        return m_string != NULL ? m_string : "";
    }

    size_type GetLength() const throw()
    {
        return m_length;
    }

    bool IsEmpty() const throw()
    {
        return (m_length == 0) || (m_string == NULL);
    }

    void Clear()
    {
        TestStringUtility::FastFree(m_string);
        m_string = NULL;
        m_length = size_type_default;
    }

    bool Find(char ch, size_type& pos, size_type start = size_type_default) const
    {
        if (m_string != NULL)
            return TestStringUtility::UnsafeFind(m_string, ch, pos, start);
        return false;
    }

    /// Extract a partial string from this string.
    /// If count is 0 then get the sub string from start to the end.
    /// Return number of characters copied.
    size_type GetSubString(TestString& sub_str,
                           size_type start,
                           size_type count) const
    {
        sub_str.Clear();
        if (start + count <= m_length)
        {
            if (count == 0)
                count = m_length - start;

            TestStringUtility::SafeReallocCopy(sub_str.m_string,
                                               sub_str.m_length,
                                               m_string + start,
                                               count);
            return count;
        }
        return 0;
    }

    /// Extract a partial string, and optionally remove the partial string
    /// from this string.
    /// If count is 0 then get the sub string from start to the end.
    void GetSubString(TestString& sub_str,
                      size_type start,
                      size_type count,
                      bool remove)
    {
        sub_str.Clear();
        count = GetSubString(sub_str, start, count);
        if ((count > 0) && remove)
        {
            size_type chars_remaining = m_length - sub_str.m_length - start;
            ::memmove(m_string + start, m_string + start + count, static_cast<size_t>(chars_remaining));
            *(m_string + start + chars_remaining) = '\0';
            m_length -= count;
            if (m_length == 0)
                TestStringUtility::SafeFree(m_string);
        }
    }

    /// Return a sub string as a TestString object.
    TestString GetSubString(size_type start, size_type count) const
    {
        TestString sub_str;
        static_cast<void>(GetSubString(sub_str, start, count));
        return sub_str;
    }

    /// Replace the current sting with str.
    void Assign(char const* str)
    {
        if ((str != NULL) && (*str != '\0'))
            TestStringUtility::SafeReallocCopy(m_string, m_length, str);
        else
            Clear();
    }

    /// Replace the current sting with str.
    void Assign(TestString const& str)
    {
        TestStringUtility::SafeReallocCopy(m_string, m_length, str.m_string);
    }

    /// Move str into this string.
    void Move(char*& str, size_type& len)
    {
        TestStringUtility::FastFree(m_string);
        m_string = str;
        m_length = len;
        str = NULL;
        len = 0U;
    }

    void Move(TestString& str)
    {
        TestStringUtility::FastFree(m_string);
        m_string = str.m_string;
        m_length = str.m_length;
        str.m_string = NULL;
        str.m_length = 0U;
    }

    void Prepend(char const* str)
    {
        if ((str != NULL) && (*str != '\0'))
        {
            TestStringUtility::SafeReallocAppend(m_string,
                                                 m_length,
                                                 str,
                                                 privateUnsafeLength(str),
                                                 m_string,
                                                 m_length);
        }
    }

    void Prepend(TestString const& str)
    {
        if ((str.m_string != NULL) && (*str.m_string != '\0'))
        {
            TestStringUtility::SafeReallocAppend(m_string,
                                                 m_length,
                                                 str.m_string,
                                                 str.m_length,
                                                 m_string,
                                                 m_length);
        }
    }

    void Append(char const* str)
    {
        if ((str != NULL) && (*str != '\0'))
        {
            TestStringUtility::SafeReallocAppend(m_string,
                                                 m_length,
                                                 m_string,
                                                 m_length,
                                                 str,
                                                 privateUnsafeLength(str));
        }
    }

    void Append(char const* str, size_type count)
    {
        if ((str != NULL) && (*str != '\0') &&
            (count > size_type_default))
        {
            TestStringUtility::SafeReallocAppend(m_string,
                                                 m_length,
                                                 m_string,
                                                 m_length,
                                                 str,
                                                 count);
        }
    }

    void Append(TestString const& str)
    {
        if ((str.m_string != NULL) &&
            (*str.m_string != '\0') &&
            (str.m_length > size_type_default))
        {
            TestStringUtility::SafeReallocAppend(m_string,
                                                 m_length,
                                                 m_string,
                                                 m_length,
                                                 str.m_string,
                                                 str.m_length);
        }
    }

    /// Append a boolean converted to a "true" ot "false" string.
    void Append(bool value)
    {
        char const* str = value ? "true" : "false";
        Append(str);
    }

    /// Append a single character.
    void Append(char value)
    {
        char str[2];
        str[0] = value;
        str[1] = '\0';
        Append(str);
    }

    /// Append number of characters.
    void Append(char value, size_type count)
    {
        if (count > size_type_default)
        {
            char* str = NULL;
            TestStringUtility::Allocate(str, count);
            if (str != NULL)
            {
                char* start = str;
                for (char* str_end = str + count; str < str_end; ++str)
                    *str = value;
                Append(start, count);
                TestStringUtility::FastFree(start);
            }
            else
                Append(value);
        }
    }

    /// Append number of characters then a string value.
    void Append(char ch, size_type count, char const* value, size_type value_len)
    {
        if (count > size_type_default)
        {
            if ((value != NULL) && (value_len > 0))
            {
                char* str = NULL;
                TestStringUtility::Allocate(str, count + value_len);
                if (str != NULL)
                {
                    char* start = str;
                    for (char* str_end = str + count; str < str_end; ++str)
                        *str = ch;
                    ::memcpy(str, value, static_cast<size_t>(value_len));
                    *(str + value_len) = '\0';
                    Append(start, count + value_len);
                    TestStringUtility::FastFree(start);
                }
                else
                    Append(ch);
            }
            else
                Append(ch, count);
        }
        else if ((value != NULL) && (value_len > 0))
            Append(value, value_len);
    }

    void Append(signed char value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(unsigned char value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(signed short value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(unsigned short value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(signed int value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(unsigned int value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(signed long value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(unsigned long value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

// Due to problems with overloading long, long long and int64 types,
// handle Microsoft C++ differently from compilers supporting long long.
#ifdef _MSC_VER

    void Append(ocl_int64 value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(ocl_uint64 value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

#else

    void Append(signed long long value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

    void Append(unsigned long long value, size_type pad = size_type_default)
    {
        privateAppendValue(value, pad);
    }

#endif // _MSC_VER

    static void EnableMemoryCounting(bool enabled)
    {
        TestStringUtility::EnableMemoryCounting(enabled);
    }

    void PadLeft(char ch, size_type count)
    {
        size_type len = GetLength();
        if (count > len)
            Prepend(TestString(ch, count - len));
    }

    void PadRight(char ch, size_type count)
    {
        size_type len = GetLength();
        if (count > len)
            Append(TestString(ch, count - len));
    }

private:
    template<typename T, typename SizeType>
    void privateAppendValue(T value, SizeType pad)
    {
        SizeType length = static_cast<SizeType>(size_type_default);
        char* str = TestConverterUtility<T>::GetString(value, length);
        if (str != NULL)
        {
            if (IsEmpty() && (pad == size_type_default))
                Move(str, length);
            else
            {
                if (pad > length)
                    Append('0', pad - length, str, length);
                else
                    Append(str, length);
                TestStringUtility::FastFree(str);
            }
        }
    }

    size_type privateUnsafeLength(char const* str)
    {
        return TestStringUtility::UnsafeLength(str);
    }

    size_type privateSafeLength(char const* str)
    {
        return TestStringUtility::SafeLength(str);
    }

private:
    size_type m_length;
    char* m_string;
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTSTRING_HPP
