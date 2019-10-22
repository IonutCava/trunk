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
#ifndef _PUSH_CONSTANTS_H_
#define _PUSH_CONSTANTS_H_

#include "PushConstant.h"

namespace Divide {
struct PushConstants {
    vectorEASTL<GFX::PushConstant> _data;

    void set(const GFX::PushConstant& constant);

    template<typename T>
    inline void set(const char* binding, GFX::PushConstantType type, const T* values, size_t count, bool flag = false) {
        U64 bindingID = _ID(binding);
        for (GFX::PushConstant& constant : _data) {
            if (constant._bindingHash == bindingID) {
                assert(constant._type == type);
                constant.set(values, count, flag);
                return;
            }
        }

        _data.emplace_back(binding, bindingID, type, values, count, flag);
    }

    template<typename T>
    inline void set(const char* binding, GFX::PushConstantType type, const T& value, bool flag = false) {
        set(binding, type, &value, 1, flag);
    }

    template<typename T>
    inline void set(const char* binding, GFX::PushConstantType type, const vector<T>& values, bool flag = false) {
        set(binding, type, values.data(), values.size(), flag);
    }

    template<typename T>
    inline void set(const char* binding, GFX::PushConstantType type, const vectorEASTL<T>& values, bool flag = false) {
        set(binding, type, values.data(), values.size(), flag);
    }

    template<typename T, size_t N>
    inline void set(const char* binding, GFX::PushConstantType type, const std::array<T, N>& values, bool flag = false) {
        set(binding, type, values.data(), N, flag);
    }

    inline void clear() { _data.clear(); }
    inline bool empty() const { return _data.empty(); }
    inline void countHint(size_t count) { _data.reserve(count); }

    inline vectorEASTL<GFX::PushConstant>& data() { return _data; }
    inline const vectorEASTL<GFX::PushConstant>& data() const { return _data; }
};

bool Merge(PushConstants& lhs, const PushConstants& rhs, bool& partial);

}; //namespace Divide

#endif //_PUSH_CONSTANTS_H_