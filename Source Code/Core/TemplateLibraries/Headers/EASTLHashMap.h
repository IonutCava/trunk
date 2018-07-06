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

#ifndef _EASTL_HASH_MAP_H_
#define _EASTL_HASH_MAP_H_
#include <EASTL/hash_map.h>


namespace hashAlg = eastl;

template <typename K, typename V, typename HashFun = HashType<K> >
using hashMapImpl = eastl::hash_map<K, V, HashFun>;

namespace eastl {

#if defined(STRING_IMP) && STRING_IMP == STL_IMP
    template <>
    struct hash<std::string> {
        size_t operator()(const std::string& x) const {
            return std::hash<std::string>()(x);
        }
    };
#endif

template <typename K, typename V, typename HashFun = HashType<K> >
using hashPairReturn =
eastl::pair<typename hashMapImpl<K, V, HashFun>::iterator, bool>;

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturn<K, V, HashFun> emplace(hashMapImpl<K, V, HashFun>& map,
    K key, const V& val) {
    hashMapImpl<K, V, HashFun>::value_type value(key);
    value.second = val;
    return map.insert(value);
}

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturn<K, V, HashFun> insert(
    hashMapImpl<K, V, HashFun>& map,
    const typename hashMapImpl<K, V, HashFun>::value_type& valuePair) {
    return map.insert(valuePair);
}

}; //namespace eastl

#endif //_EASTL_HASH_MAP_H_