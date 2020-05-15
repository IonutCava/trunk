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
#ifndef _STL_STRING_H_
#define _STL_STRING_H_

#include "TemplateAllocator.h"
#include <string>
namespace stringAlg = std;

using stringImplFast = stringAlg::basic_string<char, stringAlg::char_traits<char>, dvd_allocator<char>>;
using wstringImplFast = stringAlg::basic_string<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t>>;
using stringstreamImplFast = stringAlg::basic_stringstream<char, stringAlg::char_traits<char>, dvd_allocator<char>>;
using ostringstreamImplFast = stringAlg::basic_ostringstream<char, stringAlg::char_traits<char>, dvd_allocator<char>>;
using wstringstreamImplFast = stringAlg::basic_stringstream<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t>>;
using wostringstreamImplFast = stringAlg::basic_ostringstream<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t>>;
using istringstreamImplFast = stringAlg::basic_istringstream<char, stringAlg::char_traits<char>, dvd_allocator<char>>;
using wistringstreamImplFast = stringAlg::basic_istringstream<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t>>;
using stringbufImplFast = stringAlg::basic_stringbuf<char, stringAlg::char_traits<char>, dvd_allocator<char>>;
using wstringbufImplFast = stringAlg::basic_stringbuf<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t>>;

using stringImpl = stringAlg::basic_string<char, stringAlg::char_traits<char>, stringAlg::allocator<char>>;
using wstringImpl = stringAlg::basic_string<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>>;
using stringstreamImpl = stringAlg::basic_stringstream<char, stringAlg::char_traits<char>, stringAlg::allocator<char>>;
using ostringstreamImpl = stringAlg::basic_ostringstream<char, stringAlg::char_traits<char>, stringAlg::allocator<char>>;
using wstringstreamImpl = stringAlg::basic_stringstream<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>>;
using wostringstreamImpl = stringAlg::basic_ostringstream<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>>;
using istringstreamImpl = stringAlg::basic_istringstream<char, stringAlg::char_traits<char>, stringAlg::allocator<char>>;
using wistringstreamImpl = stringAlg::basic_istringstream<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>>;
using stringbufImpl = stringAlg::basic_stringbuf<char, stringAlg::char_traits<char>, stringAlg::allocator<char>>;
using wstringbufImpl = stringAlg::basic_stringbuf<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>>;

namespace std {
    using stringSize = size_t;

    //ref: http://www.gotw.ca/gotw/029.htm
    struct ci_char_traits : public char_traits<char>
        // just inherit all the other functions
        //  that we don't need to override
    {
        static bool eq(char c1, char c2)
        {
            return toupper(c1) == toupper(c2);
        }

        static bool ne(char c1, char c2)
        {
            return toupper(c1) != toupper(c2);
        }

        static bool lt(char c1, char c2)
        {
            return toupper(c1) < toupper(c2);
        }

        static int compare(const char* s1,
            const char* s2,
            size_t n) {
            return _memicmp(s1, s2, n);
            // if available on your compiler,
            //  otherwise you can roll your own
        }

        static const char* find(const char* s, int n, char a) {
            while (n-- > 0 && toupper(*s) != toupper(a)) {
                ++s;
            }
            return s;
        }
    }; //ci_string
}; //namespace std

using stringImpl_IgnoreCase = stringAlg::basic_string<char, stringAlg::ci_char_traits>;

#endif //_STL_STRING_H_

