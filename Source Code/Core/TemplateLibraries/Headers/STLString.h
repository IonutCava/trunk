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

#include <Allocator/stl_allocator.h>
#include <string>

namespace stringAlg = std;

typedef std::basic_string<char, std::char_traits<char>, stl_allocator<char> > stringImpl;
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, stl_allocator<wchar_t> > wstringImpl;
typedef std::basic_stringstream<char, std::char_traits<char>, stl_allocator<char> > stringstreamImpl;
typedef std::basic_ostringstream<char, std::char_traits<char>, stl_allocator<char> > ostringstreamImpl;
typedef std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, stl_allocator<wchar_t> > wstringstreamImpl;
typedef std::basic_ostringstream<wchar_t, std::char_traits<wchar_t>, stl_allocator<wchar_t> > wostringstreamImpl;
typedef std::basic_istringstream<char, std::char_traits<char>, stl_allocator<char> > istringstreamImpl;
typedef std::basic_istringstream<wchar_t, std::char_traits<wchar_t>, stl_allocator<wchar_t> > wistringstreamImpl;
typedef std::basic_stringbuf<char, std::char_traits<char>, stl_allocator<char> > stringbufImpl;
typedef std::basic_stringbuf<wchar_t, std::char_traits<wchar_t>, stl_allocator<wchar_t> > wstringbufImpl;

namespace std {
    typedef size_t stringSize;
};

template<typename T>
inline stringImpl to_stringImpl(T value) {
    return stringImpl(std::to_string(value).c_str());
}
#endif //_STL_STRING_H_
