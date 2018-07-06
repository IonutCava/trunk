/*
Copyright (c) 2016 DIVIDE-Studio
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

typedef std::basic_string<char, std::char_traits<char>, dvd_allocator<char> > stringImplFast;
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, dvd_allocator<wchar_t> > wstringImplFast;
typedef std::basic_stringstream<char, std::char_traits<char>, dvd_allocator<char> > stringstreamImplFast;
typedef std::basic_ostringstream<char, std::char_traits<char>, dvd_allocator<char> > ostringstreamImplFast;
typedef std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, dvd_allocator<wchar_t> > wstringstreamImplFast;
typedef std::basic_ostringstream<wchar_t, std::char_traits<wchar_t>, dvd_allocator<wchar_t> > wostringstreamImplFast;
typedef std::basic_istringstream<char, std::char_traits<char>, dvd_allocator<char> > istringstreamImplFast;
typedef std::basic_istringstream<wchar_t, std::char_traits<wchar_t>, dvd_allocator<wchar_t> > wistringstreamImplFast;
typedef std::basic_stringbuf<char, std::char_traits<char>, dvd_allocator<char> > stringbufImplFast;
typedef std::basic_stringbuf<wchar_t, std::char_traits<wchar_t>, dvd_allocator<wchar_t> > wstringbufImplFast;

typedef std::basic_string<char, std::char_traits<char>> stringImpl;
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>> wstringImpl;
typedef std::basic_stringstream<char, std::char_traits<char>> stringstreamImpl;
typedef std::basic_ostringstream<char, std::char_traits<char>> ostringstreamImpl;
typedef std::basic_stringstream<wchar_t, std::char_traits<wchar_t>> wstringstreamImpl;
typedef std::basic_ostringstream<wchar_t, std::char_traits<wchar_t>> wostringstreamImpl;
typedef std::basic_istringstream<char, std::char_traits<char>> istringstreamImpl;
typedef std::basic_istringstream<wchar_t, std::char_traits<wchar_t>> wistringstreamImpl;
typedef std::basic_stringbuf<char, std::char_traits<char>> stringbufImpl;
typedef std::basic_stringbuf<wchar_t, std::char_traits<wchar_t>> wstringbufImpl;


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
};

template<typename T>
inline stringImpl to_stringImpl(T value) {
    return stringImpl(std::to_string(value).c_str());
}
#endif //_STL_STRING_H_
