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
#ifndef _GL_RESOURCES_INL_
#define _GL_RESOURCES_INL_

namespace Divide {
namespace GLUtil {

template<size_t N>
void glTexturePool<N>::init()
{
    glGenTextures(N, _handles.data());
    _usageMap.fill(State::FREE);
}

template<size_t N>
void glTexturePool<N>::clean() {
    U32 count = to_U32(eastl::count_if(eastl::begin(_usageMap), eastl::end(_usageMap), [](State state) { return state == State::CLEAN; }));
    if (count > to_U32(_usageMap.size() / 2)) {
        glDeleteTextures(count, _handles.data());
        glGenTextures(count, _handles.data());
        eastl::fill(eastl::begin(_usageMap), eastl::begin(_usageMap) + count, State::FREE);
    }
}

template<size_t N>
void glTexturePool<N>::destroy() {
    glDeleteTextures(N, _handles.data());
    _handles.fill(0);
    _usageMap.fill(State::CLEAN);
}

template<size_t N>
GLuint glTexturePool<N>::allocate(bool retry) {
    for (size_t i = 0; i < N; ++i) {
        if (_usageMap[i] == State::FREE) {
            _usageMap[i] = State::USED;
            return _handles[i];
        }
    }
    if (!retry) {
        clean();
        return allocate(true);
    }

    DIVIDE_UNEXPECTED_CALL();
    return 0;
}

template<size_t N>
void glTexturePool<N>::deallocate(GLuint& handle) {
    for (size_t i = 0; i < N; ++i) {
        if (_handles[i] == handle) {
            handle = 0;
            _usageMap[i] = State::CLEAN;
            return;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
}

}; // namespace GLUtil
}; // namespace Divide
#endif  //_GL_RESOURCES_INL_