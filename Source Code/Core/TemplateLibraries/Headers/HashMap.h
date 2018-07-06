/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <algorithm>

template<class T, bool>
struct hasher {
    inline size_t operator() (const T& elem) {
        return std::hash<T>()(elem);
    }
};

template<class T>
struct hasher<T, true> {
    inline size_t operator() (const T& elem) {
        typedef typename std::underlying_type<T>::type enumType;
        return std::hash<enumType>()(static_cast<enumType>(elem));
    }
};

template<class T>
struct EnumHash {
    inline size_t operator()(const T& elem) const {
        return hasher<T, std::is_enum<T>::value>()(elem);
    }
};


template <typename Key>
using HashType = EnumHash<Key>;

#if defined(HASH_MAP_IMP) && HASH_MAP_IMP == BOOST_IMP
#include "BOOSTHashMap.h"
#elif defined(HASH_MAP_IMP) && HASH_MAP_IMP == EASTL_IMP
#include "EASTLHashMap.h"
#else  // defined(HASH_MAP_IMP) && HASH_MAP_IMP == STL_IMP
#include "STLHashMap.h"
#endif

#endif
