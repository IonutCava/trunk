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
#ifndef _STRING_H_
#define _STRING_H_

#include <EASTL/string.h>
#include <EASTL/fixed_string.h>
#include "STLString.h"

namespace Divide {
    template<size_t Length>
    class Str final : public eastl::fixed_string<char, Length + 1, false>
    {
    public:
        using Base = eastl::fixed_string<char, Length + 1, false>;

        using Base::npos;
        using Base::mPair;
        using Base::append;
        using Base::resize;
        using Base::clear;
        using Base::capacity;
        using Base::size;
        using Base::sprintf_va_list;
        using Base::DoAllocate;
        using Base::DoFree;
        using Base::internalLayout;
        using Base::get_allocator;
        using Base::fixed_string;

        Str() : Str("")
        {
        }

        template<typename T_str>
        Str(const T_str& str) : Base(str.c_str())
        {
        }

        ~Str() = default;

        inline Str& operator+(const char* other) {
            append(other);
            return *this;
        }

        inline Str operator+(const char* other) const {
            Str ret(*this);
            ret.append(other);
            return ret;
        }

        template<size_t N>
        inline Str& operator+(const Str<N>& other) {
            return *this + other.c_str();
        }

        template<size_t N>
        inline Str operator+(const Str<N>& other) const {
            return *this + other.c_str();
        }

        template<typename T_str>
        inline Str& operator+(const T_str& other) {
            return *this + other.c_str();
        }

        template<typename T_str>
        inline Str operator+(const T_str& other) const {
            return *this + other.c_str();
        }

        template<typename T_str>
        inline operator T_str() {
            return T_str{ this->c_str() };
        }
    };

    using Str8   = Str<8>;
    using Str16  = Str<16>;
    using Str32  = Str<32>;
    using Str64  = Str<64>;
    using Str128 = Str<128>;
    using Str256 = Str<256>;

};//namespace Divide
#endif //_STRING_H_
