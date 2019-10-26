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
#include <boost/beast/core/static_string.hpp>
#include <boost/utility/string_ref.hpp >
#include "STLString.h"

namespace Divide {
    template<size_t Length>
    class Str final : public boost::beast::static_string<Length>
    {
    public:
        using Base = boost::beast::static_string<Length>;

        using Base::static_string;
        using Base::append;

        Str() : Base() {}

        template<typename T_str,
                 typename std::enable_if<std::is_same<stringImpl, T_str>::value || 
                                         std::is_same<stringImplFast, T_str>::value, I32>::type = 0>
        Str(const T_str& str) : Base(str.c_str(), str.length())
        {
        }

        template<size_t N>
        Str(const boost::beast::static_string<N>& other) : Base(other.c_str(), other.size())
        {
            static_assert(N <= Length, "Invalid static string size!");
        }

        template<size_t N>
        Str(const Str<N>& other) : Base(other.c_str(), other.size())
        {
            static_assert(N <= Length, "Fixed String construction: Constructing a smaller fixed string from a larger one!");
        }

        ~Str() = default;

        boost::string_ref as_ref(size_t pos = 0) const {
            if (pos == 0) {
                return boost::string_ref(Base::c_str(), Base::size());
            }
            auto subStr = Base::substr(pos, Base::size() - pos);
            return boost::string_ref(subStr.data(), subStr.length());
        }

        inline Str operator+(const char* other) const {
            Str ret(*this);
            ret.append(other);
            return ret;
        }

        template<size_t N>
        inline Str operator+(const Str<N>& other) const {
            Str ret(*this);
            ret.append(other);
            return ret;
        }

        template<typename T_str>
        typename std::enable_if<std::is_same<stringImpl, T_str>::value || 
                                std::is_same<stringImplFast, T_str>::value, Str>::type
        operator+(const T_str& other) const {
            return *this + other.c_str();
        }

        template<typename T_str, typename std::enable_if_t<std::is_same<stringImpl, T_str>::value ||
                                                           std::is_same<stringImplFast, T_str>::value>::value>
        inline operator T_str() {
            return T_str{ this->c_str() };
        }

        template<typename T_str>
        typename std::enable_if<std::is_same<stringImpl, T_str>::value ||
                                std::is_same<stringImplFast, T_str>::value, Str&>::type
        append(const T_str& other) {
            *this = Str((this->c_str() + other).c_str());
            return *this;
        }

        template<size_t N>
        inline Str& append(const Str<N>& other) {
            this->append(other.c_str());
            return *this;
        }

        template<size_t N>
        inline Str append(const Str<N>& other) const {
            Str ret(*this);
            ret.append(other.c_str());
            return ret;
        }

        inline Str& append(const char* other) {
            Base::append(other);
            return *this;
        }

        template<typename T_str, typename std::enable_if<std::is_same<stringImpl, T_str>::value || 
                                                         std::is_same<stringImplFast, T_str>::value>::type>
        inline Str& operator=(const T_str& other) {
            Base::assign(other.c_str(), other.length());
            return *this;
        }

        operator stringImpl() const {
            return stringImpl(Base::c_str());
        }

        operator stringImplFast() const {
            return stringImplFast(Base::c_str());
        }

        inline size_t find(char other, size_t pos = 0) const {
            return pos + as_ref(pos).find(other, pos);
            return ret == Str::npos ? ret : ret + pos;
        }

        inline size_t find(const char* other, size_t pos = 0) const {
            size_t ret = as_ref(pos).find(boost::string_ref(other));
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        inline size_t find(const Str& other, size_t pos = 0) const {
            size_t ret = as_ref(pos).find(other.as_ref());
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        template<size_t N>
        inline size_t find(const Str<N>& other, size_t pos = 0) const {
            size_t ret = as_ref(pos).find(other.as_ref());
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        inline size_t rfind(char other, size_t pos = 0) const {
            size_t ret = as_ref(pos).rfind(other);
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        inline size_t rfind(const char* other, size_t pos = 0) const {
            size_t ret = as_ref(pos).rfind(boost::string_ref(other));
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        inline size_t rfind(const Str& other, size_t pos = 0) const {
            size_t ret = as_ref(pos).rfind(other.as_ref());
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        template<size_t N>
        inline size_t rfind(const Str<N>& other, size_t pos = 0) const {
            size_t ret = as_ref(pos).rfind(other.as_ref());
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        inline size_t find_first_of(char s, size_t pos = 0) const {
            size_t ret = as_ref(pos).find_first_of(s);
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        inline size_t find_first_of(const char* s, size_t pos = 0) const {
            size_t ret = as_ref(pos).find_first_of(boost::string_ref(s));
            return ret != Str::npos ? ret + pos : Str::npos;
        }

        inline Str substr(size_t start, size_t count) const {
            if (count > 0) {
                count = std::min(count, Base::size());
                const char* data = Base::substr(start, count).data();
                return Str(data, count);
            }

            return "";
        }

        inline Str& replace(size_t start, size_t length, const char* s) {
            //Barf
            *this = stringImpl(Base::c_str()).replace(start, length, s).c_str();
            return *this;
        }
    };


    template<size_t N, size_t M>
    Str<N> operator+(const Str<N>& lhs, const Str<M>& rhs) {
        return lhs.append(rhs);
    }

    template<size_t N>
    Str<N> operator+(const char* lhs, const Str<N>& rhs) {
        Str<N> ret(lhs);
        return ret + rhs;
    }

    template<size_t N>
    Str<N> operator+(char lhs, const Str<N>& rhs) {
        rhs.append(rhs.begin(), lhs);
        return rhs;
    }

    template<size_t N>
    Str<N> operator+(const Str<N>& lhs, const char* rhs) {
        Str<N> ret(lhs);
        ret.append(rhs);
        return ret;
    }

    template<size_t N>
    Str<N> operator+(const Str<N>& lhs, char rhs) {
        Str<N> ret(lhs);
        ret.append(lhs.end(), rhs);
        return ret;
    }

    using Str8   = Str<8>;
    using Str16  = Str<16>;
    using Str32  = Str<32>;
    using Str64  = Str<64>;
    using Str128 = Str<128>;
    using Str256 = Str<256>;
};//namespace Divide
#endif //_STRING_H_
