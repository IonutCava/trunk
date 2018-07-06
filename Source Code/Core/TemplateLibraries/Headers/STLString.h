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

#ifndef _STL_STRING_H_
#define _STL_STRING_H_

#include "TemplateAllocator.h"
#include <string>

namespace stringAlg = std;

typedef stringAlg::basic_string<char, stringAlg::char_traits<char>, dvd_allocator<char> > stringImplFast;
typedef stringAlg::basic_string<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t> > wstringImplFast;
typedef stringAlg::basic_stringstream<char, stringAlg::char_traits<char>, dvd_allocator<char> > stringstreamImplFast;
typedef stringAlg::basic_ostringstream<char, stringAlg::char_traits<char>, dvd_allocator<char> > ostringstreamImplFast;
typedef stringAlg::basic_stringstream<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t> > wstringstreamImplFast;
typedef stringAlg::basic_ostringstream<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t> > wostringstreamImplFast;
typedef stringAlg::basic_istringstream<char, stringAlg::char_traits<char>, dvd_allocator<char> > istringstreamImplFast;
typedef stringAlg::basic_istringstream<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t> > wistringstreamImplFast;
typedef stringAlg::basic_stringbuf<char, stringAlg::char_traits<char>, dvd_allocator<char> > stringbufImplFast;
typedef stringAlg::basic_stringbuf<wchar_t, stringAlg::char_traits<wchar_t>, dvd_allocator<wchar_t> > wstringbufImplFast;

typedef stringAlg::basic_string<char, stringAlg::char_traits<char>, stringAlg::allocator<char>> stringImpl;
typedef stringAlg::basic_string<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>> wstringImpl;
typedef stringAlg::basic_stringstream<char, stringAlg::char_traits<char>, stringAlg::allocator<char>> stringstreamImpl;
typedef stringAlg::basic_ostringstream<char, stringAlg::char_traits<char>, stringAlg::allocator<char>> ostringstreamImpl;
typedef stringAlg::basic_stringstream<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>> wstringstreamImpl;
typedef stringAlg::basic_ostringstream<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>> wostringstreamImpl;
typedef stringAlg::basic_istringstream<char, stringAlg::char_traits<char>, stringAlg::allocator<char>> istringstreamImpl;
typedef stringAlg::basic_istringstream<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>> wistringstreamImpl;
typedef stringAlg::basic_stringbuf<char, stringAlg::char_traits<char>, stringAlg::allocator<char>> stringbufImpl;
typedef stringAlg::basic_stringbuf<wchar_t, stringAlg::char_traits<wchar_t>, stringAlg::allocator<wchar_t>> wstringbufImpl;


#if defined(USE_CUSTOM_MEMORY_ALLOCATORS)
typedef stringImplFast stringImplBest;
typedef wstringImplFast wstringImplBest;
typedef stringstreamImplFast stringstreamImplBest;
typedef ostringstreamImplFast ostringstreamImplBest;
typedef wstringstreamImplFast wstringstreamImplBest;
typedef wostringstreamImplFast wostringstreamImplBest;
typedef istringstreamImplFast istringstreamImplBest;
typedef wistringstreamImplFast wistringstreamImplBest;
typedef stringbufImplFast stringbufImplBest;
typedef wstringbufImplFast wstringbufImplBest;
#else
typedef stringImpl stringImplBest;
typedef wstringImpl wstringImplBest;
typedef stringstreamImpl stringstreamImplBest;
typedef ostringstreamImpl ostringstreamImplBest;
typedef wstringstreamImpl wstringstreamImplBest;
typedef wostringstreamImpl wostringstreamImplBest;
typedef istringstreamImpl istringstreamImplBest;
typedef wistringstreamImpl wistringstreamImplBest;
typedef stringbufImpl stringbufImplBest;
typedef wstringbufImpl wstringbufImplBest;
#endif

namespace std {
    typedef size_t stringSize;

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

        static const char*
            find(const char* s, int n, char a) {
            while (n-- > 0 && toupper(*s) != toupper(a)) {
                ++s;
            }
            return s;
        }
    }; //ci_string
}; //namespace std

template<typename T>
inline stringImpl to_stringImpl(T value) {
    return stringImpl(std::to_string(value).c_str());
}

typedef stringAlg::basic_string<char, stringAlg::ci_char_traits> stringImpl_IgnoreCase;

#endif //_STL_STRING_H_

