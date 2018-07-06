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

#pragma once
#ifndef _VECTOR_H_
#define _VECTOR_H_

#include "TemplateAllocator.h"

#include <EASTL/sort.h>
#include <EASTL/vector.h>
#include <EASTL/allocator.h>
#include <EASTL/fixed_allocator.h>
#include <vector>

namespace vectorAlg = std;

template <typename Type, typename Allocator = vectorAlg::allocator<Type>>
using vector = std::vector<Type, Allocator>;

template <typename Type>
using vectorFast = std::vector<Type, dvd_allocator<Type>>;

template <typename Type>
using vectorEASTL = eastl::vector<Type>;

#if defined(USE_CUSTOM_MEMORY_ALLOCATORS)
template <typename Type>
using vectorBest = vectorFast<Type>;
#else
template <typename Type>
using vectorBest = vector<Type>;
#endif

typedef std::size_t vec_size;
typedef eastl_size_t vec_size_eastl;


template< typename T, typename Pred, typename A>
typename vector<T, A>::iterator insert_sorted(vector<T, A>& vec, T const& item, Pred pred)
{
    return vec.insert(std::upper_bound(std::begin(vec), std::end(vec), item, pred), item);
}

template<typename T, typename A>
void insert_unique(vector<T, A>& target, const T& item)
{
    if (std::find(std::cbegin(target), std::cend(target), item) != std::cend(target))
    {
        target.push_back(item);
    }
}

template<typename T, typename A>
void insert_unique(vector<T, A>& target, const vector<T, A>& source)
{
    std::for_each(std::cbegin(source), std::cend(source),
        [&target](T const& item) {
            insert_unique(target, item);
        });
}


template<typename T>
void insert_unique(vectorEASTL<T>& target, const T& item)
{
    if (eastl::find(eastl::cbegin(target), eastl::cend(target), item) != eastl::cend(target))
    {
        target.push_back(item);
    }

}
template<typename T>
void insert_unique(vectorEASTL<T>& target, const vectorEASTL<T>& source)
{
    eastl::for_each(eastl::cbegin(source), eastl::cend(source),
        [&target](T const& item) {
        insert_unique(target, item);
    });
}

template<typename T, typename A>
void pop_front(vector<T, A>& vec)
{
    assert(!vec.empty());
    vec.erase(std::begin(vec));
}

template<typename T, typename A>
void unchecked_copy(vector<T, A>& dst, const vector<T, A>& src)
{
    dst.resize(src.size());
    memcpy(dst.data(), src.data(), src.size() * sizeof(T));
}

template<typename T, typename U, typename A>
vector<T, A> convert(const vector<U, A>& data) {
    return vector<T, A>(std::cbegin(data), std::cend(data));
}

//ref: https://stackoverflow.com/questions/7571937/how-to-delete-items-from-a-stdvector-given-a-list-of-indices
template<typename T, typename A>
inline typename vector<T, A> erase_indices(const typename vector<T, A>& data, vector<vec_size>& indicesToDelete)
{
    if (indicesToDelete.empty()) {
        return data;
    }

    vector<T, A> ret;
    ret.reserve(data.size() - indicesToDelete.size());

    std::sort(std::cbegin(indicesToDelete), std::cend(indicesToDelete));

    // new we can assume there is at least 1 element to delete. copy blocks at a time.
    vector<T, A>::const_iterator itBlockBegin = std::cbegin(data);
    for (vector<vec_size, A>::const_iterator it = std::cbegin(indicesToDelete); it != std::cend(indicesToDelete); ++it)  {
        vector<T, A>::const_iterator itBlockEnd = std::cbegin(data) + *it;
        if (itBlockBegin != itBlockEnd) {
            std::copy(itBlockBegin, itBlockEnd, std::back_inserter(ret));
        }
        itBlockBegin = itBlockEnd + 1;
    }

    // copy last block.
    if (itBlockBegin != data.end()) {
        std::copy(itBlockBegin, std::cend(data), std::back_inserter(ret));
    }

    return ret;
}

template<typename T>
inline vector<T> erase_indices(const vector<T>& data, vector<vec_size>& indicesToDelete/* can't assume copy elision, don't pass-by-value */)
{
    if (indicesToDelete.empty()) {
        return data;
    }

    vector<T> ret;
    ret.reserve(data.size() - indicesToDelete.size());

    std::sort(std::begin(indicesToDelete), std::end(indicesToDelete));

    // new we can assume there is at least 1 element to delete. copy blocks at a time.
    vector<T>::const_iterator itBlockBegin = std::cbegin(data);
    for (vector<vec_size>::const_iterator it = std::cbegin(indicesToDelete); it != std::cend(indicesToDelete); ++it) {
        vector<T>::const_iterator itBlockEnd = std::cbegin(data) + *it;
        if (itBlockBegin != itBlockEnd) {
            std::copy(itBlockBegin, itBlockEnd, std::back_inserter(ret));
        }
        itBlockBegin = itBlockEnd + 1;
    }

    // copy last block.
    if (itBlockBegin != data.end()) {
        std::copy(itBlockBegin, std::cend(data), std::back_inserter(ret));
    }

    return ret;
}

template<typename T>
inline vectorEASTL<T> erase_indices(const vectorEASTL<T>& data, vector<vec_size>& indicesToDelete/* can't assume copy elision, don't pass-by-value */)
{
    if (indicesToDelete.empty()) {
        return data;
    }

    vectorEASTL<T> ret;
    ret.reserve(data.size() - indicesToDelete.size());

    eastl::sort(eastl::begin(indicesToDelete), eastl::end(indicesToDelete));

    // new we can assume there is at least 1 element to delete. copy blocks at a time.
    vectorEASTL<T>::const_iterator itBlockBegin = eastl::cbegin(data);
    for (auto it = eastl::cbegin(indicesToDelete); it != eastl::cend(indicesToDelete); ++it) {
        vectorEASTL<T>::const_iterator itBlockEnd = eastl::cbegin(data) + *it;
        if (itBlockBegin != itBlockEnd) {
            eastl::copy(itBlockBegin, itBlockEnd, eastl::back_inserter(ret));
        }
        itBlockBegin = itBlockEnd + 1;
    }

    // copy last block.
    if (itBlockBegin != data.end()) {
        eastl::copy(itBlockBegin, eastl::cend(data), eastl::back_inserter(ret));
    }

    return ret;
}
#endif
