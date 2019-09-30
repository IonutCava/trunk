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
#ifndef _PUSH_CONSTANT_H_
#define _PUSH_CONSTANT_H_

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
        FCOLOUR3,
        FCOLOUR4,
        //MAT_N_x_M,
        COUNT
    };

    struct PushConstant {
        PushConstant() = default;
        ~PushConstant() = default;

        PushConstant(const PushConstant& other) = default;
        PushConstant& operator=(const PushConstant& other) = default;
        PushConstant(PushConstant&& other) noexcept = default;
        PushConstant& operator=(PushConstant&& other) noexcept = default;

        template<typename T>
        PushConstant(const char* binding,
                     U64 bindingHash,
                     PushConstantType type,
                     const T& value,
                     bool flag = false)
            : _binding(binding),
              _bindingHash(bindingHash),
              _type(type),
              _flag(flag)
        {
            _buffer.resize(sizeof(T));
            std::memcpy(_buffer.data(), &value, _buffer.size());
        }

        template<typename T>
        PushConstant(const char* binding,
                     U64 bindingHash,
                     PushConstantType type,
                     const vector<T>& values,
                     bool flag = false)
            : _binding(binding),
              _bindingHash(bindingHash),
              _type(type),
              _flag(flag)
        {
            if (!values.empty()) {
                _buffer.resize(values.size() * (sizeof(T)));
                std::memcpy(_buffer.data(), values.data(), _buffer.size());
            }
        }

        template<typename T>
        PushConstant(const char* binding,
                     U64 bindingHash,
                     PushConstantType type,
                     const vectorEASTL<T>& values,
                     bool flag = false)
            : _binding(binding),
              _bindingHash(bindingHash),
              _type(type),
              _flag(flag)
        {
            if (!values.empty()) {
                _buffer.resize(values.size() * (sizeof(T)));
                std::memcpy(_buffer.data(), values.data(), _buffer.size());
            }
        }

        template<>
        PushConstant(const char* binding,
                     U64 bindingHash,
                     PushConstantType type,
                     const vectorEASTL<bool>& values,
                     bool flag)
            : _binding(binding),
              _bindingHash(bindingHash),
              _type(type),
              _flag(flag)
        {
            if (!values.empty()) {
                _buffer.reserve(values.size());
                std::transform(std::cbegin(values), std::cend(values),
                               std::back_inserter(_buffer),
                               [](bool e) {return to_byte(e ? 1 : 0);});
            }
        }

        template<typename T, size_t N>
        PushConstant(const char* binding,
                     U64 bindingHash,
                     PushConstantType type,
                     const std::array<T, N>& values,
                     bool flag = false)
            : _binding(binding),
              _bindingHash(bindingHash),
              _type(type),
              _flag(flag)
        {
            if (!values.empty()) {
                _buffer.resize(values.size() * (sizeof(T)));
                std::memcpy(_buffer.data(), values.data(), _buffer.size());
            }
        }

        template<size_t N>
        PushConstant(const char* binding,
                     U64 bindingHash,
                     PushConstantType type,
                     const std::array<bool, N>& values,
                     bool flag)
            : _binding(binding),
              _bindingHash(bindingHash),
              _type(type),
              _flag(flag)
        {
            if (!values.empty()) {
                _buffer.reserve(N);
                std::transform(std::cbegin(values), std::cend(values),
                               std::back_inserter(_buffer),
                               [](bool e) {return to_byte(e ? 1 : 0);});
            }
        }

        template<typename T>
        inline void set(const T& value, bool flag = false) {
            _flag = flag;
            std::memcpy(_buffer.data(), &value, _buffer.size());
        }

        template<typename T>
        inline void set(const vector<T>& values, bool flag = false) {
            _flag = flag;
            if (values.empty()) {
                _buffer.resize(values.size() * (sizeof(T)));
                std::memcpy(_buffer.data(), values.data(), _buffer.size());
            }
        }

        template<typename T>
        inline void set(const vectorEASTL<T>& values, bool flag = false) {
            _flag = flag;
            if (!values.empty()) {
                _buffer.resize(values.size() * (sizeof(T)));
                std::memcpy(_buffer.data(), values.data(), _buffer.size());
            }
        }

        template<>
        inline void set(const vectorEASTL<bool>& values, bool flag) {
            _flag = flag;

            if (!values.empty()) {
                _buffer.reserve(values.size());
                std::transform(std::cbegin(values), std::cend(values),
                    std::back_inserter(_buffer),
                    [](bool e) {return to_byte(e ? 1 : 0); });
            }
        }

        template<typename T, size_t N>
        inline void set(const std::array<T, N>& values, bool flag = false){
            _flag = flag;

            if (!values.empty()) {
                _buffer.resize(values.size() * (sizeof(T)));
                std::memcpy(_buffer.data(), values.data(), _buffer.size());
            }
        }

        template<size_t N>
        inline void set(const std::array<bool, N>& values, bool flag) {
            _flag = flag;

            if (!values.empty()) {
                _buffer.reserve(N);
                std::transform(std::cbegin(values), std::cend(values),
                    std::back_inserter(_buffer),
                    [](bool e) {return to_byte(e ? 1 : 0); });
            }
        }


        void clear();

        inline bool operator==(const PushConstant& rhs) const {
            return _type == rhs._type &&
                   _flag == rhs._flag &&
                   _bindingHash == rhs._bindingHash &&
                   _buffer == rhs._buffer;
        }

        inline bool operator!=(const PushConstant& rhs) const {
            return _type != rhs._type || 
                   _flag != rhs._flag ||
                   _bindingHash != rhs._bindingHash ||
                   _buffer != rhs._buffer;
        }

        union {
            bool _flag = false;
            bool _transpose;
        };

        eastl::fixed_string<char, 64 + 1, false> _binding;
        vectorEASTL<char> _buffer;
        U64               _bindingHash;
        PushConstantType  _type = PushConstantType::COUNT;
    };
}; //namespace GFX
}; //namespace Divide
#endif //_PUSH_CONSTANT_H_