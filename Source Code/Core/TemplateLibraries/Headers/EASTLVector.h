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

#ifndef _EASTL_VECTOR_H_
#define _EASTL_VECTOR_H_
#ifndef EA_COMPILER_HAS_MOVE_SEMANTICS
#define EA_COMPILER_HAS_MOVE_SEMANTICS
#endif
#include <EASTL/vector.h>
#include <Allocator/stl_allocator.h>

namespace vectorAlg = eastl;

template <typename Type>
using vectorImpl = vectorAlg::vector<Type, stl_allocator<Type>>;

template <typename Type>
using vectorImplAligned = vectorAlg::vector<Type>;

namespace eastl {
    typedef eastl_size_t vecSize;

    template <typename T>
    inline void shrinkToFit(vectorImpl<T>& inputVector) {
        inputVector.set_capacity(inputVector.size() * sizeof(T));
    }

    template <typename T, class... Args>
    inline void emplace_back(vectorImpl<T>& inputVector,
        Args&&... args) {
        new (inputVector.push_back_uninitialized()) T(std::forward<Args>(args)...);
    }

    template <typename T>
    inline void shrinkToFit(vectorImplAligned<T>& inputVector) {
        inputVector.set_capacity(inputVector.size() * sizeof(T));
    }

    template <typename T, class... Args>
    inline void emplace_back(vectorImplAligned<T>& inputVector,
        Args&&... args) {
        new (inputVector.push_back_uninitialized()) T(std::forward<Args>(args)...);
    }

};

#endif //_EASTL_VECTOR_H_