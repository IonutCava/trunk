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

#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <algorithm>

template<class T>
struct EnumHash;

template <typename Key>
using HashType = EnumHash<Key>;

#include <Allocator/stl_allocator.h>

#if defined(HASH_MAP_IMP) && HASH_MAP_IMP == BOOST_IMP
#include <boost/Unordered_Map.hpp>
// macros are evil but we need to extend namespaces so aliases don't work as well
#define hashAlg boost
#elif defined(HASH_MAP_IMP) && HASH_MAP_IMP == EASTL_IMP
#include <EASTL/hash_map.h>
#define hashAlg eastl
#else  // defined(HASH_MAP_IMP) && HASH_MAP_IMP == STL_IMP
#include <unordered_map>
#define hashAlg std
#endif


template <typename K, typename V, typename HashFun = HashType<K> >
using hashMapImpl = hashAlg::unordered_map<K,
                                           V,
                                           HashFun,
                                           std::equal_to<K>,
                                           stl_allocator<std::pair<const K, V>>>;

template <typename K, typename V, typename HashFun = HashType<K> >
using hashMapImplAligned = hashAlg::unordered_map<K,
                                                  V,
                                                  HashFun>;

template <typename K, typename V, typename HashFun = HashType<K> >
using hashPairReturn = std::pair<typename hashMapImpl<K, V, HashFun>::iterator, bool>;

template <typename K, typename V, typename HashFun = HashType<K> >
using hashPairReturnAligned = std::pair<typename hashMapImplAligned<K, V, HashFun>::iterator, bool>;

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

namespace hashAlg {
#if defined(HASH_MAP_IMP) && HASH_MAP_IMP == EASTL_IMP
template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturn<K, V, HashFun> emplace(hashMapImpl<K, V, HashFun>& map, K key, const V& val) {
    hashMapImpl<K, V, HashFun>::value_type value(key);
    value.second = val;

    auto result = map.insert(value);

    return std::make_pair(result.first, result.second);
}

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturn<K, V, HashFun> insert(hashMapImpl<K, V, HashFun>& map, const std::pair<K, V>& valuePair) {
    
    auto result = map.insert(eastl::pair<K, V>(valuePair.first, valuePair.second));

    return std::make_pair(result.first, result.second);
}

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturnAligned<K, V, HashFun> emplace(hashMapImplAligned<K, V, HashFun>& map, K key, const V& val) {
    hashMapImpl<K, V, HashFun>::value_type value(key);
    value.second = val;

    auto result = map.insert(value);

    return std::make_pair(result.first, result.second);
}

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturnAligned<K, V, HashFun> insert(hashMapImplAligned<K, V, HashFun>& map, const std::pair<K, V>& valuePair) {

    auto result = map.insert(eastl::pair<K, V>(valuePair.first, valuePair.second));

    return std::make_pair(result.first, result.second);
}

#else 
template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturn<K, V, HashFun> emplace(hashMapImpl<K, V, HashFun>& map, K key, const V& val) {
    return map.emplace(key, val);
}

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturn<K, V, HashFun> insert(hashMapImpl<K, V, HashFun>& map, const typename hashMapImpl<K, V, HashFun>::value_type& valuePair) {
    return map.insert(valuePair);
}

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturnAligned<K, V, HashFun> emplace(hashMapImplAligned<K, V, HashFun>& map, K key, const V& val) {
    return map.emplace(key, val);
}

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturnAligned<K, V, HashFun> insert(hashMapImplAligned<K, V, HashFun>& map, const typename hashMapImpl<K, V, HashFun>::value_type& valuePair) {
    return map.insert(valuePair);
}
#endif
}; //namespace hashAlg

#endif
