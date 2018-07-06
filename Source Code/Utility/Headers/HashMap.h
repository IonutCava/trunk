/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include "String.h"

#if defined(HASH_MAP_IMP) && HASH_MAP_IMP == 0
    #include <boost/Unordered_Map.hpp>
    
    namespace hashAlg = boost;

    template<typename K, typename V, typename HashFun = boost::hash<K> >
    using hashMapImpl = boost::unordered_map<K, V, HashFun>;

    namespace boost {

#   if defined(STRING_IMP) && STRING_IMP == 0
        template<>
        struct hash<stringImpl>
        {
            size_t operator()(const stringImpl& v) const {
                return std::hash<std::string>()(v.c_str());
            }
        };
#   endif

        template<typename K, typename V, typename HashFun = boost::hash<K> >
        using hashPairReturn = std::pair<typename hashMapImpl<K, V, HashFun>::iterator, bool>;

        template<typename T1, typename T2>
        using pair = std::pair<T1, T2>;

        template<typename K, typename V, typename HashFun = hashAlg::hash<K> >
        inline hashPairReturn<K, V, HashFun> emplace(hashMapImpl<K, V, HashFun>& map, 
                                                     K key, 
                                                     const V& val) {
            return map.emplace(key, val);
        }

        template<typename K, typename V, typename HashFun = hashAlg::hash<K> >
        inline hashPairReturn<K, V, HashFun> insert(hashMapImpl<K, V, HashFun>& map,
                                                    const typename hashMapImpl<K, V, HashFun>::value_type& valuePair) {
            return map.insert(valuePair);
        }

        template<typename K, typename V, typename HashFun = hashAlg::hash<K> >
        inline void fastClear(hashMapImpl<K, V,HashFun>& map){
            map.clear();
        }

#    ifndef BOOST_PAIR_FUNCS
#    define BOOST_PAIR_FUNCS
            template<typename K, typename V>
            inline std::pair<K, V> makePair(const K& key, const V& val) {
                return std::make_pair(key, val);
            }

            template<typename K, typename V>
            inline std::pair<K, V> makePairCpy(const K& key, V val) {
                return std::make_pair(key, val);
            }
#    endif
    };

#elif defined(HASH_MAP_IMP) && HASH_MAP_IMP == 1

    #include <EASTL/hash_map.h>

    namespace hashAlg = eastl;

    template<typename K, typename V, typename HashFun = eastl::hash<K> >
    using hashMapImpl = eastl::hash_map<K, V, HashFun>;

    namespace eastl {

        template<>
        struct hash<std::string>
        {
            size_t operator()(const std::string & x) const
            {
                return std::hash<std::string>()(x);
            }
        };

        template<typename K, typename V, typename HashFun = eastl::hash<K> >
        using hashPairReturn = eastl::pair<typename hashMapImpl<K, V, HashFun>::iterator, bool>;

        template<typename K, typename V, typename HashFun = eastl::hash<K> >
        inline hashPairReturn<K, V, HashFun> emplace(hashMapImpl<K, V, HashFun>& map,
                                                     K key, 
                                                     const V& val) {
            hashMapImpl<K, V, HashFun>::value_type value(key);
            value.second = val;
            return map.insert(value); 
        }
    
        template<typename K, typename V, typename HashFun = hashAlg::hash<K> >
        inline hashPairReturn<K, V, HashFun> insert(hashMapImpl<K, V, HashFun>& map,
                                                    const typename hashMapImpl<K, V, HashFun>::value_type& valuePair) {
            return map.insert(valuePair);
        }
    
        template<typename K, typename V, typename HashFun = hashAlg::hash<K> >
        inline void fastClear(hashMapImpl<K, V, HashFun>& map) {
            //map.reset(); // leaks memory -Ionut
            map.clear();
        }

#    ifndef EASTL_PAIR_FUNCS
#    define EASTL_PAIR_FUNCS
            template<typename K, typename V>
            inline eastl::pair<K, V> makePair(const K& key, const V& val) {
                return eastl::make_pair_ref(key, val);
            }
   
            template<typename K, typename V>
            inline eastl::pair<K, V> makePairCpy(const K& key, V val) {
                return eastl::make_pair_ref(key, val);
            }
#    endif
    };

#else //defined(HASH_MAP_IMP) && HASH_MAP_IMP == 2
    #include <unordered_map>

    namespace hashAlg = std;

    template<typename K, typename V, typename HashFun = std::hash<K> >
    using hashMapImpl = std::unordered_map<K, V, HashFun>;

    namespace std {

        template<>
        struct hash<eastl::string>
        {
            size_t operator()(const eastl::string & x) const
            {
                return std::hash<std::string>()(x.c_str());
            }
        };

        template<typename K, typename V, typename HashFun = std::hash<K> >
        using hashPairReturn = std::pair<typename hashMapImpl<K, V, HashFun>::iterator, bool>;

        template<typename K, typename V, typename HashFun = hashAlg::hash<K> >
        inline hashPairReturn<K, V, HashFun> emplace(hashMapImpl<K, V, HashFun>& map,
                                                    K key, 
                                                    const V& val) {
            return map.emplace(key, val);
        }

        template<typename K, typename V, typename HashFun = hashAlg::hash<K> >
        inline hashPairReturn<K, V, HashFun> insert(hashMapImpl<K, V, HashFun>& map, 
                                                    const typename hashMapImpl<K, V, HashFun>::value_type& valuePair) {
            return map.insert(valuePair);
        }

        template<typename K, typename V, typename HashFun = hashAlg::hash<K> >
        inline void fastClear(hashMapImpl<K, V, HashFun>& map){
            map.clear();
        }

#    ifndef STD_PAIR_FUNCS
#    define STD_PAIR_FUNCS
            template<typename K, typename V>
            inline std::pair<K, V> makePair(const K& key, const V& val) {
                return std::make_pair(key, val);
            }

            template<typename K, typename V>
            inline std::pair<K, V> makePairCpy(const K& key, V val) {
                return std::make_pair(key, val);
            }
#    endif
    };

#endif //defined(HASH_MAP_IMP)
    
#endif