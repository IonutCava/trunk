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

#ifndef VECTOR_H_
#define VECTOR_H_

#include "config.h"

/// boost vectors have a completely different interface, so we can't actually use them
#if defined(VECTOR_IMP) && VECTOR_IMP == 2
#	undef  VECTOR_IMP
#	define VECTOR_IMP 0
#endif

#if defined(VECTOR_IMP) && VECTOR_IMP == 2
/// vector.hpp has some issues in 1.55 (fixed in 1.56)
/*
#include <boost/container/vector.hpp>

namespace vectorAlg = boost;

template<typename Type>
using vectorImpl = boost::container::vector<Type>;

namespace boost {
    typedef size_t vecSize;

    template<typename T>
    inline void shrinkToFit(vectorImpl<T>& inputVector) {
        inputVector.shrink_to_fit();
    }

#	ifndef BOOST_PAIR_FUNCS
#	define BOOST_PAIR_FUNCS
		template<typename K, typename V>
		inline std::pair<K, V> makePair(const K& key, const V& val) {
		return std::make_pair(key, val);
		}

		template<typename K, typename V>
		inline std::pair<K, V> makePairCpy(const K& key, V val) {
		return std::make_pair(key, val);
		}
#	endif

};
*/
#elif defined(VECTOR_IMP) && VECTOR_IMP == 0

#include <EASTL/vector.h>

namespace vectorAlg  = eastl;

template<typename Type>
using vectorImpl = vectorAlg::vector<Type>;

namespace eastl {
    typedef eastl_size_t vecSize;

    template<typename T>
    inline void shrinkToFit(vectorImpl<T>& inputVector) {
        inputVector.set_capacity(inputVector.size()  * sizeof(T));
    }

#	ifndef EASTL_PAIR_FUNCS
#	define EASTL_PAIR_FUNCS
		template<typename K, typename V>
		inline eastl::pair<K, V> makePair(const K& key, const V& val) {
			return eastl::make_pair_ref(key, val);
		}

		template<typename K, typename V>
		inline eastl::pair<K, V> makePairCpy(const K& key, V val) {
			return eastl::make_pair_ref(key, val);
		}
#	endif

};

#else //defined(VECTOR_IMP) && VECTOR_IMP == 1

#include <vector>

namespace vectorAlg = std;

template<typename Type>
using vectorImpl = vectorAlg::vector<Type>;

namespace std {
    typedef size_t vecSize;

	template<typename T1, typename T2>
	using pair = std::pair<T1, T2>;

    template<typename T>
    inline void shrinkToFit(vectorImpl<T>& inputVector) {
        inputVector.shrink_to_fit();
    }

#	ifndef STD_PAIR_FUNCS
#	define STD_PAIR_FUNCS
		template<typename K, typename V>
		inline std::pair<K, V> makePair(const K& key, const V& val) {
			return std::make_pair(key, val);
		}

		template<typename K, typename V>
		inline std::pair<K, V> makePairCpy(const K& key, V val) {
			return std::make_pair(key, val);
		}
#	endif
};
#endif //defined(VECTOR_IMP)

#endif