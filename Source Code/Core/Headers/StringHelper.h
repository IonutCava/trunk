/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _CORE_STRING_HELPER_H_
#define _CORE_STRING_HELPER_H_

#include "Platform/Headers/PlatformDefines.h"
#include "fmt/format.h"
#include "fmt/printf.h"

namespace Divide {
    namespace Util {
        bool findCommandLineArgument(int argc, char** argv, const char* target_arg, const char* arg_prefix = "--");

        template<typename T_str = stringImpl>
        void ReplaceStringInPlace(T_str& subject, const stringImpl& search, const stringImpl& replace, bool recursive = false);

        template<typename T_str = stringImpl>
        T_str ReplaceString(const T_str& subject, const stringImpl& search, const stringImpl& replace, bool recursive = false);

        template<typename T_str = stringImpl>
        void GetPermutations(const T_str& inputString, vector<T_str>& permutationContainer);

        template<typename T_str = stringImpl>
        bool IsNumber(const T_str& s);

        bool IsNumber(const char* s);

        template<typename T_str = stringImpl>
        T_str GetTrailingCharacters(const T_str& input, size_t count);

        template<typename T_str = stringImpl>
        T_str GetStartingCharacters(const T_str& input, size_t count);

        template<class FwdIt, class Compare = std::less<typename std::iterator_traits<FwdIt>::value_type>>
        void InsertionSort(FwdIt first, FwdIt last, Compare cmp = Compare());

        template<typename T_strA = stringImpl, typename T_strB = stringImpl>
        bool CompareIgnoreCase(const T_strA& a, const T_strB& b);

        bool CompareIgnoreCase(const char* a, const char* b);

        /// http://stackoverflow.com/questions/236129/split-a-string-in-c
        template<typename T_vec, typename T_str = stringImpl>
        typename std::enable_if<std::is_same<T_vec, vector<T_str>>::value ||
                                std::is_same<T_vec, vectorFast<T_str>>::value ||
                                std::is_same<T_vec, vectorEASTL<T_str>>::value, T_vec>::type
        Split(const char* input, char delimiter);

        template<typename T_vec, typename T_str = stringImpl>
        typename std::enable_if<std::is_same<T_vec, vector<T_str>>::value ||
                                std::is_same<T_vec, vectorFast<T_str>>::value, T_vec&>::type
        Split(const char* input, char delimiter, T_vec& elems);

        template<typename T_str = stringImpl> vectorEASTL<T_str>&
        Split(const char* input, char delimiter, vectorEASTL<T_str>& elems);

        /// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
        template<typename T_str = stringImpl>
        T_str& Ltrim(T_str& s);

        template<typename T_str = stringImpl>
        T_str Ltrim(const T_str& s);

        template<typename T_str = stringImpl>
        T_str& Rtrim(T_str& s);

        template<typename T_str = stringImpl>
        T_str Rtrim(const T_str& s);

        template<typename T_str = stringImpl>
        T_str& Trim(T_str& s);

        template<typename T_str = stringImpl>
        T_str  Trim(const T_str& s);

        ALIAS_TEMPLATE_FUNCTION(StringFormat, fmt::sprintf)

        template<typename T_str = stringImpl>
        U32 LineCount(const T_str& str);

        void CStringRemoveChar(char* str, char charToRemove) noexcept;
    }; //namespace Util
}; //namespace Divide

#endif //_CORE_STRING_HELPER_H_

#include "StringHelper.inl"