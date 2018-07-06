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

#ifndef _PUSH_CONSTANTS_H_
#define _PUSH_CONSTANTS_H_

#include "Platform/Headers/PlatformDataTypes.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

enum class PushConstantType : U8 {
    BOOL = 0,
    INT,
    UINT,
    FLOAT,
    DOUBLE,
    //BVEC2, use vec2<I32>(1/0, 1/0)
    //BVEC3, use vec3<I32>(1/0, 1/0)
    //BVEC4, use vec4<I32>(1/0, 1/0)
    IVEC2,
    IVEC3,
    IVEC4,
    UVEC2,
    UVEC3,
    UVEC4,
    VEC2,
    VEC3,
    VEC4,
    DVEC2,
    DVEC3,
    DVEC4,
    MAT2,
    MAT3,
    MAT4,
    //MAT_N_x_M,
    COUNT
};

struct PushConstant {
    //I32              _binding = -1;
    stringImplFast   _binding;
    PushConstantType _type = PushConstantType::COUNT;
    vectorImplFast<AnyParam> _values;
    union {
        bool _flag = false;
        bool _transpose;
    };
};

class PushConstants {
public:
    PushConstants()
    {
    }

    PushConstants(const PushConstant& constant)
    {
        _data[_ID_RT(constant._binding.c_str())] = constant;
    }

    PushConstants(const vectorImpl<PushConstant>& data)
    {
        for (const PushConstant& constant : data) {
            _data[_ID_RT(constant._binding.c_str())] = constant;
        }
    }

    inline void set(const stringImplFast& binding,
                    PushConstantType type,
                    const AnyParam& value,
                    bool flag = false) {
        vectorImplFast<AnyParam> values{ value };
        _data[_ID_RT(binding.c_str())] = { binding, type, values, flag };
    }

    inline void set(const stringImplFast& binding,
                    PushConstantType type,
                    const vectorImplFast<AnyParam>& values,
                    bool flag = false) {
        _data[_ID_RT(binding.c_str())] = { binding, type, values, flag };
    }

    inline bool empty() const { return _data.empty(); }

    inline hashMapImpl<U64, PushConstant>& data() { return _data; }

    inline const hashMapImpl<U64, PushConstant>& data() const { return _data; }

protected:
    hashMapImpl<U64, PushConstant> _data;
};

};

#endif //_PUSH_CONSTANTS_H_