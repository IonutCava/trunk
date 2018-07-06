/*
   Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _VECTOR_H_
#define _VECTOR_H_

/// boost vectors have a completely different interface,
/// so we can't actually use them
#if defined(VECTOR_IMP) && VECTOR_IMP == BOOST_IMP
#undef VECTOR_IMP
#define VECTOR_IMP STL_IMP
#endif

/// EASTL vectors can't erase entries using const_iterators and also have forwarding issues
/// so we can't actually use them
#if defined(VECTOR_IMP) && VECTOR_IMP == EASTL_IMP
#undef VECTOR_IMP
#define VECTOR_IMP STL_IMP
#endif

#if defined(VECTOR_IMP) && VECTOR_IMP == BOOST_IMP
#include "BoostVector.h"
#elif defined(VECTOR_IMP) && VECTOR_IMP == EASTL_IMP
#include "EASTLVector.h"
#else  // defined(VECTOR_IMP) && VECTOR_IMP == STL_IMP
#include "STLVector.h"
#endif  // defined(VECTOR_IMP)

template< typename T, typename Pred >
typename vectorImpl<T>::iterator insert_sorted(vectorImpl<T>& vec, T const& item, Pred pred)
{
    return vec.insert(std::upper_bound(std::begin(vec), std::end(vec), item, pred), item);
}

template<typename T>
void pop_front(vectorImpl<T>& vec)
{
    assert(!vec.empty());
    vec.erase(std::begin(vec));
}

template<typename T>
void unchecked_copy(vectorImpl<T>& dst, const vectorImpl<T>& src)
{
    dst.resize(src.size());
    memcpy(dst.data(), src.data(), src.size() * sizeof(T));
}

//ref: https://stackoverflow.com/questions/7571937/how-to-delete-items-from-a-stdvector-given-a-list-of-indices
template<typename T>
inline vectorImplFast<T> erase_indices(const vectorImplFast<T>& data, vectorImplFast<vectorAlg::vecSize>& indicesToDelete/* can't assume copy elision, don't pass-by-value */)
{
    if (indicesToDelete.empty()) {
        return data;
    }

    vectorImplFast<T> ret;
    ret.reserve(data.size() - indicesToDelete.size());

    std::sort(std::begin(indicesToDelete), std::end(indicesToDelete));

    // new we can assume there is at least 1 element to delete. copy blocks at a time.
    vectorImplFast<T>::const_iterator itBlockBegin = std::cbegin(data);
    for (vectorImplFast<size_t>::const_iterator it = std::cbegin(indicesToDelete); it != std::cend(indicesToDelete); ++it)  {
        vectorImplFast<T>::const_iterator itBlockEnd = std::cbegin(data) + *it;
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

#endif
