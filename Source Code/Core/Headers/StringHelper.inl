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

#ifndef FMT_EXCEPTIONS
#define FMT_EXCEPTIONS 0
#endif

#include "fmt/format.h"
#include "fmt/printf.h"

#ifndef _CORE_STRING_HELPER_INL_
#define _CORE_STRING_HELPER_INL_

namespace Divide {
namespace Util {

template <typename... Args>
stringImpl StringFormat(const char *const format, Args&&... args) {
    return fmt::sprintf(format, std::forward<Args>(args)...);
#if 0 //Ref
    int sz = snprintf(nullptr, 0, format, std::forward<Args>(args)...);
    vectorEASTL<char> buf(vec_size_eastl(sz) + 1, '\0');
    snprintf(&buf[0], buf.size(), format, std::forward<Args>(args)...);
    return stringImpl(buf.data(), buf.size() - 1);
#endif
}

template<typename T_vec, typename T_str>
typename std::enable_if<std::is_same<T_vec, typename vector<T_str>>::value ||
                        std::is_same<T_vec, typename vectorFast<T_str>>::value, T_vec&>::type
Split(const char* input, char delimiter, T_vec& elems) {
    elems.resize(0);
    if (input != nullptr) {
        istringstreamImpl ss(input);
        stringImpl item;
        while (std::getline(ss, item, delimiter)) {
            elems.push_back(std::move(item));
        }
    }

    return elems;
}

template<typename T_str>
vectorEASTL<T_str>& Split(const char* input, char delimiter, vectorEASTL<T_str>& elems) {
    elems.resize(0);
    if (input != nullptr && strlen(input) > 0) {
        istringstreamImpl ss(input);
        stringImpl item;
        while (std::getline(ss, item, delimiter)) {
            elems.emplace_back(item.c_str());
        }
    }

    return elems;
}

template<typename T_vec, typename T_str>
typename std::enable_if<std::is_same<T_vec, vector<T_str>>::value ||
                        std::is_same<T_vec, vectorFast<T_str>>::value ||
                        std::is_same<T_vec, vectorEASTL<T_str>>::value, T_vec>::type
Split(const char* input, char delimiter) {
    T_vec elems;
    return Split<T_vec, T_str>(input, delimiter, elems);
}

/// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
inline stringImpl Ltrim(const stringImpl& s) {
    stringImpl temp(s);
    return Ltrim(temp);
}

inline stringImpl& Ltrim(stringImpl& s) {
    s.erase(std::begin(s),
        std::find_if(std::begin(s), std::end(s),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

inline stringImpl Rtrim(const stringImpl& s) {
    stringImpl temp(s);
    return Rtrim(temp);
}

inline stringImpl& Rtrim(stringImpl& s) {
    s.erase(
        std::find_if(std::rbegin(s), std::rend(s),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
        std::end(s));
    return s;
}

inline stringImpl& Trim(stringImpl& s) {
    return Ltrim(Rtrim(s));
}

inline stringImpl Trim(const stringImpl& s) {
    stringImpl temp(s);
    return Trim(temp);
}

}; //namespace Util
}; //namespace Divide
#endif //_CORE_STRING_HELPER_INL_