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

#ifndef _PUSH_CONSTANT_H_
#define _PUSH_CONSTANT_H_

#include "Platform/Headers/PlatformDataTypes.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {
namespace GFX {

    //ToDo: Make this more generic. Also used by the Editor -Ionut
    enum class PushConstantType : U8 {
        BOOL = 0,
        INT,
        UINT,
        FLOAT,
        DOUBLE,
        //BVEC2, use vec2<I32>(1/0, 1/0)
        //BVEC3, use vec3<I32>(1/0, 1/0, 1/0)
        //BVEC4, use vec4<I32>(1/0, 1/0, 1/0, 1/0)
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
              _bindingHash(_ID_RT(binding.c_str())),
              _type(type),
              _flag(flag),
              _transpose(false)
        {
            _buffer.resize(values.size() * (sizeof(T)));
            if (!_buffer.empty()) {
                std::memcpy(_buffer.data(), values.data(), _buffer.size());
            }
        }

        template<>
        PushConstant(const stringImplFast& binding,
                     PushConstantType type,
                     const vectorImpl<bool>& values,
                     bool flag)
            : _binding(binding),
              _bindingHash(_ID_RT(binding.c_str())),
              _type(type),
              _flag(flag),
              _transpose(false)
        {
            _buffer.reserve(values.size());
            if (!_buffer.empty()) {
                std::transform(std::cbegin(values), std::cend(values),
                               std::back_inserter(_buffer),
                               [](bool e) {return to_byte(e ? 1 : 0);});
            }
        }

        template<typename T, size_t N>
        PushConstant(const stringImplFast& binding,
                     PushConstantType type,
                     const std::array<T, N>& values,
                     bool flag = false)
            : _binding(binding),
              _bindingHash(_ID_RT(binding.c_str())),
              _type(type),
              _flag(flag),
              _transpose(false)
        {
            _buffer.resize(values.size() * (sizeof(T)));
            if (!_buffer.empty()) {
                std::memcpy(_buffer.data(), values.data(), _buffer.size());
            }
        }

        template<size_t N>
        PushConstant(const stringImplFast& binding,
                     PushConstantType type,
                     const std::array<bool, N>& values,
                     bool flag)
            : _binding(binding),
              _bindingHash(_ID_RT(binding.c_str()))
              _type(type),
              _flag(flag),
              _transpose(false)
        {
            _buffer.reserve(N);
            if (!_buffer.empty()) {
                std::transform(std::cbegin(values), std::cend(values),
                               std::back_inserter(_buffer),
                               [](bool e) {return to_byte(e ? 1 : 0);});
            }
        }

        PushConstant(const PushConstant& other);
        PushConstant& operator=(const PushConstant& other);
        PushConstant& assign(const PushConstant& other);

        ~PushConstant();

        void clear();


        inline bool operator==(const PushConstant& rhs) const {
            return _bindingHash == rhs._bindingHash &&
                   _type == rhs._type &&
                   _flag == rhs._flag &&
                   _buffer == rhs._buffer;
        }

        inline bool operator!=(const PushConstant& rhs) const {
            return _bindingHash != rhs._bindingHash ||
                   _type != rhs._type ||
                   _flag != rhs._flag ||
                   _buffer != rhs._buffer;
        }

        U64              _bindingHash;
        stringImplFast   _binding;
        PushConstantType _type = PushConstantType::COUNT;
        vectorImpl<char> _buffer;
        union {
            bool _flag = false;
            bool _transpose;
        };
    };
}; //namespace GFX
}; //namespace Divide
#endif //_PUSH_CONSTANT_H_