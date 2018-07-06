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
#ifndef _EASTL_VECTOR_H_
#define _EASTL_VECTOR_H_

#include <EASTL/vector.h>
#include <EASTL/allocator.h>
#include <EASTL/fixed_allocator.h>
#include "TemplateAllocator.h"

namespace vectorAlg = eastl;

template <typename Type>
using vector = eastl::vector<Type>;

//ToDo: actually create a better general purpose allocator
template <typename Type>
using vectorFast = vector<Type>;

#if defined(USE_CUSTOM_MEMORY_ALLOCATORS)
template <typename Type>
using vectorBest = vectorFast<Type>;
#else
template <typename Type>
using vectorBest = vector<Type>;
#endif

typedef eastl_size_t vec_size;

namespace eastl {

    template <typename T>
    inline void shrinkToFit(vector<T>& inputVector) {
        inputVector.shrink_to_fit();
    }

    template <typename T, class... Args>
    inline void emplace_back(vector<T>& inputVector, Args&&... args) {
        inputVector.emplace_back(std::forward<Args>(args)...);
    }

};

#endif //_EASTL_VECTOR_H_