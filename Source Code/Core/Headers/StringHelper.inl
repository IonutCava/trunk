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

#ifndef _CORE_STRING_HELPER_INL_
#define _CORE_STRING_HELPER_INL_

namespace Divide {
namespace Util {

template<typename T>
typename std::enable_if<std::is_same<T, vector<stringImpl>>::value ||
                        std::is_same<T, vectorFast<stringImpl>>::value, T&>::type
Split(const stringImpl& input, char delimiter, T& elems) {
#if defined(_USE_BOOST_STRING_SPLIT)
    boost::split(elems, input, [delimiter](char c) {return c == delimiter;});
#else
    elems.resize(0);
    if (!input.empty()) {
        istringstreamImpl ss(input);
        stringImpl item;
        while (std::getline(ss, item, delimiter)) {
            vectorAlg::emplace_back(elems, item);
        }
    }
#endif
    return elems;
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