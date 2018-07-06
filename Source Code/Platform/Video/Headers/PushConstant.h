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

#ifndef _PUSH_CONSTANT_H_
#define _PUSH_CONSTANT_H_

#include "Platform/Headers/PlatformDataTypes.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {
namespace GFX {

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
        IMAT2,
        IMAT3,
        IMAT4,
        UMAT2,
        UMAT3,
        UMAT4,
        MAT2,
        MAT3,
        MAT4,
        DMAT2,
        DMAT3,
        DMAT4,
        //MAT_N_x_M,
        COUNT
    };

    struct PushConstant {
        PushConstant() = default;

        template<typename T>
        PushConstant(const stringImplFast& binding,
                     PushConstantType type,
                     const vectorImpl<T>& values,
                     bool flag = false)
            : _binding(binding),
            _type(type),
            _values(std::cbegin(values), std::cend(values)),
            _flag(flag),
            _transpose(false)
        {
        }

        template<typename T, size_t N>
        PushConstant(const stringImplFast& binding,
                     PushConstantType type,
                     const std::array<T, N>& values,
                     bool flag = false)
            : _binding(binding),
            _type(type),
            _values(std::cbegin(values), std::cend(values)),
            _flag(flag),
            _transpose(false)
        {
        }

        PushConstant(const PushConstant& other)
        {
            assign(other);
        }

        PushConstant& operator=(const PushConstant& other) {
            return assign(other);
        }

        PushConstant& assign(const PushConstant& other) {
            _binding = other._binding;
            _type = other._type;
            _flag = other._flag;
            _values = other._values;

            return *this;
        }

        ~PushConstant() {
            clear();
        }

        inline void clear() {
            _values.clear();
            _binding.clear();
            _type = PushConstantType::COUNT;
            _flag = false;
        }

        //I32              _binding = -1;
        stringImplFast   _binding;
        PushConstantType _type = PushConstantType::COUNT;
        vectorImpl<AnyParam> _values;
        union {
            bool _flag = false;
            bool _transpose;
        };
    };
}; //namespace GFX
}; //namespace Divide
#endif //_PUSH_CONSTANT_H_