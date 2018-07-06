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
class PushConstants {
    public:
    PushConstants();
    PushConstants(const GFX::PushConstant& constant);
    PushConstants(const vectorImpl<GFX::PushConstant>& data);
    ~PushConstants();

    PushConstants(const PushConstants& other);
    PushConstants& operator=(const PushConstants& other);

    void set(const GFX::PushConstant& constant);

    template<typename T>
    inline void set(const stringImplFast& binding,
                    GFX::PushConstantType type,
                    const T& value,
                    bool flag = false) {
        set(binding, type, vectorImpl<T>{value}, flag);
    }

    template<typename T>
    inline void set(const stringImplFast& binding,
                    GFX::PushConstantType type,
                    const vectorImpl<T>& values,
                    bool flag = false) {

        U64 bindingID = _ID_RT(binding.c_str());
        for (GFX::PushConstant& constant : _data) {
            if (constant._bindingHash == bindingID) {
                constant.assign({ binding, type, values, flag });
                return;
            }
        }

        _data.emplace_back(binding, type, values, flag );
    }

    template<typename T, size_t N>
    inline void set(const stringImplFast& binding,
                    GFX::PushConstantType type,
                    const std::array<T, N>& values,
                    bool flag = false) {

        U64 bindingID = _ID_RT(binding.c_str());
        for (GFX::PushConstant& constant : _data) {
            if (constant._bindingHash == bindingID) {
                constant.assign({ binding, type, values, flag });
                return;
            }
        }

        _data.emplace_back(binding, type, values, flag);
    }

    void clear();

    inline bool empty() const { return _data.empty(); }

    inline vectorImpl<GFX::PushConstant>& data() { return _data; }

    inline const vectorImpl<GFX::PushConstant>& data() const { return _data; }

    bool merge(const PushConstants& other);

  protected:
    vectorImpl<GFX::PushConstant> _data;
};

}; //namespace Divide

#endif //_PUSH_CONSTANTS_H_