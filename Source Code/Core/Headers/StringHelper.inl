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

#ifndef _CORE_STRING_HELPER_INL_
#define _CORE_STRING_HELPER_INL_

namespace Divide {
namespace Util {

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

template<typename T_str>
bool IsNumber(const T_str& s) {
    return IsNumber(s.c_str());
}

template<typename T_str>
void GetPermutations(const T_str& inputString, vector<T_str>& permutationContainer) {
    permutationContainer.clear();
    T_str tempCpy(inputString);
    std::sort(std::begin(tempCpy), std::end(tempCpy));
    do {
        permutationContainer.push_back(inputString);
    } while (std::next_permutation(std::begin(tempCpy), std::end(tempCpy)));
}

template<typename T_str>
inline void ReplaceStringInPlace(T_str& subject,
                                 const stringImpl& search,
                                 const stringImpl& replace,
                                 bool recursive) {
    bool changed = true;
    while (changed) {
        changed = false;

        size_t pos = 0;
        while ((pos = subject.find(search.c_str(), pos)) != T_str::npos) {
            subject.replace(pos, search.length(), replace.c_str());
            pos += replace.length();
            changed = true;
        }

        if (!recursive) {
            break;
        }
    }
}

template<typename T_str>
inline T_str ReplaceString(const T_str& subject,
                           const stringImpl& search,
                           const stringImpl& replace,
                           bool recursive)
{
    T_str ret = subject;
    ReplaceStringInPlace(ret, search, replace, recursive);
    return ret;
}

template<typename T_str>
T_str GetTrailingCharacters(const T_str& input, size_t count) {
    const size_t inputLength = input.length();
    count = std::min(inputLength, count);
    assert(count > 0);
    return input.substr(inputLength - count, inputLength);
}

template<typename T_str>
T_str GetStartingCharacters(const T_str& input, size_t count) {
    const size_t inputLength = input.length();
    count = std::min(inputLength, count);
    assert(count > 0);
    return input.substr(0, inputLength - count);
}


inline bool CompareIgnoreCase(const char* a, const char* b) {
    assert(a != nullptr && b != nullptr);
    return strcasecmp(a, b) == 0;
}

template<typename T_strA>
inline bool CompareIgnoreCase(const T_strA& a, const char* b) {
    if (b != nullptr && !a.empty()) {
        return CompareIgnoreCase(a.c_str(), b);
    }
    return false;
}

template<>
inline bool CompareIgnoreCase(const stringImpl& a, const stringImpl& b) {
    if (a.length() == b.length()) {
        return std::equal(std::cbegin(b),
                          std::cend(b),
                          std::cbegin(a),
            [](unsigned char a, unsigned char b) {
                return a == b || std::tolower(a) == std::tolower(b);
            });
    }

    return false;
}

template<>
inline bool CompareIgnoreCase(const stringImplFast& a, const stringImplFast& b) {
    if (a.length() == b.length()) {
        return std::equal(std::cbegin(b),
                          std::cend(b),
                          std::cbegin(a),
            [](unsigned char a, unsigned char b) {
                return a == b || std::tolower(a) == std::tolower(b);
            });
    }

    return false;
}

template<typename T_strA, typename T_strB>
inline bool CompareIgnoreCase(const T_strA& a, const T_strB& b) {
    return a.comparei(b.c_str()) == 0;
}

template<typename T_str>
U32 LineCount(const T_str& str) {
    return to_U32(std::count(std::cbegin(str), std::cend(str), '\n')) + 1;
}

/// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
template<typename T_str>
inline T_str Ltrim(const T_str& s) {
    T_str temp(s);
    return Ltrim(temp);
}

template<typename T_str>
inline T_str& Ltrim(T_str& s) {
    s.erase(std::begin(s),
        std::find_if(std::begin(s), std::end(s),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

template<typename T_str>
inline T_str Rtrim(const T_str& s) {
    T_str temp(s);
    return Rtrim(temp);
}

template<typename T_str>
inline T_str& Rtrim(T_str& s) {
    s.erase(
        std::find_if(std::rbegin(s), std::rend(s),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
        std::end(s));
    return s;
}

template<typename T_str>
inline T_str& Trim(T_str& s) {
    return Ltrim(Rtrim(s));
}

template<typename T_str>
inline T_str Trim(const T_str& s) {
    T_str temp(s);
    return Trim(temp);
}

}; //namespace Util
}; //namespace Divide
#endif //_CORE_STRING_HELPER_INL_