/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _CORE_STRING_HELPER_H_
#define _CORE_STRING_HELPER_H_

#include "Platform/Headers/PlatformDefines.h"
#include <sstream>

namespace Divide {
    namespace Util {
        void ReplaceStringInPlace(stringImpl& subject, const stringImpl& search, const stringImpl& replace);

        void GetPermutations(const stringImpl& inputString, vectorImpl<stringImpl>& permutationContainer);

        bool IsNumber(const stringImpl& s);

        stringImpl GetTrailingCharacters(const stringImpl& input, size_t count);

        stringImpl GetStartingCharacters(const stringImpl& input, size_t count);

        template<class FwdIt, class Compare = std::less<typename std::iterator_traits<FwdIt>::value_type>>
        void InsertionSort(FwdIt first, FwdIt last, Compare cmp = Compare());

        bool CompareIgnoreCase(const stringImpl& a, const stringImpl& b);

        /// http://stackoverflow.com/questions/236129/split-a-string-in-c
        vectorImpl<stringImpl> Split(const stringImpl& input, char delimiter);

        vectorImpl<stringImpl>& Split(const stringImpl& input, char delimiter, vectorImpl<stringImpl>& elems);

        /// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
        stringImpl& Ltrim(stringImpl& s);
        stringImpl Ltrim(const stringImpl& s);

        stringImpl& Rtrim(stringImpl& s);
        stringImpl Rtrim(const stringImpl& s);

        stringImpl& Trim(stringImpl& s);
        stringImpl  Trim(const stringImpl& s);

        //format is passed by value to conform with the requirements of va_start.
        //ref: http://codereview.stackexchange.com/questions/115760/use-va-list-to-format-a-string
        stringImpl StringFormat(const char *const format, ...);

        U32 LineCount(const stringImpl& str);

        void CStringRemoveChar(char* str, char charToRemove);
    }; //namespace Util
}; //namespace Divide

#endif //_CORE_STRING_HELPER_H_

#include "StringHelper.inl"