/*
* Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include "ByteConverter.h"
#include <vector>
#include <list>
#include <map>

#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>

namespace Divide {

namespace Networking {
template <typename T>
class vector3 {
   public:
    explicit vector3(T x, T y, T z) : _x(x), _y(y), _z(z) {}
    T _x, _y, _z;
};
};

class ByteBufferException {
   public:
    ByteBufferException(bool _add, size_t _pos, size_t _esize, size_t _size)
        : add(_add), pos(_pos), esize(_esize), size(_size) {
        PrintPosError();
    }

    void PrintPosError() const {}

   private:
    bool add;
    size_t pos;
    size_t esize;
    size_t size;
};

template <typename T>
struct Unused {
    Unused() {}
};

class ByteBuffer {
   public:
    const static size_t DEFAULT_SIZE = 0x1000;

    // constructor
    ByteBuffer() : _rpos(0), _wpos(0) { _storage.reserve(DEFAULT_SIZE); }

    // constructor
    ByteBuffer(size_t res) : _rpos(0), _wpos(0) { _storage.reserve(res); }

    // copy constructor
    ByteBuffer(const ByteBuffer &buf)
        : _rpos(buf._rpos), _wpos(buf._wpos), _storage(buf._storage) {}

    void clear() {
        _storage.clear();
        _rpos = _wpos = 0;
    }

    template <typename T>
    void put(size_t pos, T value) {
        EndianConvert(value);
        put(pos, (U8 *)&value, sizeof(value));
    }

    ByteBuffer &operator<<(U8 value) {
        append<U8>(value);
        return *this;
    }

    ByteBuffer &operator<<(U16 value) {
        append<U16>(value);
        return *this;
    }

    ByteBuffer &operator<<(U32 value) {
        append<U32>(value);
        return *this;
    }

    ByteBuffer &operator<<(U64 value) {
        append<U64>(value);
        return *this;
    }

    // signed as in 2e complement
    ByteBuffer &operator<<(I8 value) {
        append<I8>(value);
        return *this;
    }

    ByteBuffer &operator<<(I16 value) {
        append<I16>(value);
        return *this;
    }

    ByteBuffer &operator<<(I32 value) {
        append<I32>(value);
        return *this;
    }

    ByteBuffer &operator<<(I64 value) {
        append<I64>(value);
        return *this;
    }

    // floating points
    ByteBuffer &operator<<(F32 value) {
        append<F32>(value);
        return *this;
    }

    ByteBuffer &operator<<(D32 value) {
        append<D32>(value);
        return *this;
    }

    ByteBuffer &operator<<(const stringImpl &value) {
        append((U8 const *)value.c_str(), value.length());
        append((U8)0);
        return *this;
    }

    ByteBuffer &operator<<(const char *str) {
        append((U8 const *)str, str ? strlen(str) : 0);
        append((U8)0);
        return *this;
    }

    ByteBuffer &operator>>(bool &value) {
        value = read<char>() > 0 ? true : false;
        return *this;
    }

    ByteBuffer &operator>>(U8 &value) {
        value = read<U8>();
        return *this;
    }

    ByteBuffer &operator>>(U16 &value) {
        value = read<U16>();
        return *this;
    }

    ByteBuffer &operator>>(U32 &value) {
        value = read<U32>();
        return *this;
    }

    ByteBuffer &operator>>(U64 &value) {
        value = read<U64>();
        return *this;
    }

    // signed as in 2e complement
    ByteBuffer &operator>>(I8 &value) {
        value = read<I8>();
        return *this;
    }

    ByteBuffer &operator>>(I16 &value) {
        value = read<I16>();
        return *this;
    }

    ByteBuffer &operator>>(I32 &value) {
        value = read<I32>();
        return *this;
    }

    ByteBuffer &operator>>(I64 &value) {
        value = read<I64>();
        return *this;
    }

    ByteBuffer &operator>>(F32 &value) {
        value = read<F32>();
        return *this;
    }

    ByteBuffer &operator>>(D32 &value) {
        value = read<D32>();
        return *this;
    }

    ByteBuffer &operator>>(stringImpl& value) {
        value.clear();
        while (rpos() <
               size())  // prevent crash at wrong string format in packet
        {
            char c = read<char>();
            if (c == 0) break;
            value += c;
        }
        return *this;
    }

    template <typename T>
    ByteBuffer &operator>>(Unused<T> const &) {
        read_skip<T>();
        return *this;
    }

    U8 operator[](size_t pos) const { return read<U8>(pos); }

    size_t rpos() const { return _rpos; }

    size_t rpos(size_t rpos_) {
        _rpos = rpos_;
        return _rpos;
    }

    size_t wpos() const { return _wpos; }

    size_t wpos(size_t wpos_) {
        _wpos = wpos_;
        return _wpos;
    }

    template <typename T>
    void read_skip() {
        read_skip(sizeof(T));
    }

    void read_skip(size_t skip) {
        if (_rpos + skip > size())
            throw ByteBufferException(false, _rpos, skip, size());
        _rpos += skip;
    }

    template <typename T>
    T read() {
        T r = read<T>(_rpos);
        _rpos += sizeof(T);
        return r;
    }

    template <typename T>
    T read(size_t pos) const {
        if (pos + sizeof(T) > size())
            throw ByteBufferException(false, pos, sizeof(T), size());
        T val = *((T const *)&_storage[pos]);
        EndianConvert(val);
        return val;
    }

    void read(U8 *dest, size_t len) {
        if (_rpos + len > size())
            throw ByteBufferException(false, _rpos, len, size());
        memcpy(dest, &_storage[_rpos], len);
        _rpos += len;
    }

    U64 readPackGUID() {
        U64 guid = 0;
        U8 guidmark = 0;
        (*this) >> guidmark;

        for (I32 i = 0; i < 8; ++i) {
            if (guidmark & (static_cast<U8>(1) << i)) {
                U8 bit;
                (*this) >> bit;
                guid |= (static_cast<U64>(bit) << (i * 8));
            }
        }

        return guid;
    }

    const U8 *contents() const { return &_storage[0]; }

    size_t size() const { return _storage.size(); }
    bool empty() const { return _storage.empty(); }

    void resize(size_t newsize) {
        _storage.resize(newsize);
        _rpos = 0;
        _wpos = size();
    }

    void reserve(size_t ressize) {
        if (ressize > size()) _storage.reserve(ressize);
    }

    void append(const stringImpl& str) {
        append((U8 const *)str.c_str(), str.size() + 1);
    }

    void append(const char *src, size_t cnt) {
        return append((const U8 *)src, cnt);
    }

    template <typename T>
    void append(const T *src, size_t cnt) {
        return append((const U8 *)src, cnt * sizeof(T));
    }

    void append(const U8 *src, size_t cnt);

    void append(const ByteBuffer &buffer) {
        if (buffer.wpos()) append(buffer.contents(), buffer.wpos());
    }

    // can be used in SMSG_MONSTER_MOVE opcode
    void appendPackXYZ(F32 x, F32 y, F32 z) {
        U32 packed = 0;
        packed |= (to_int(x / 0.25f) & 0x7FF);
        packed |= (to_int(y / 0.25f) & 0x7FF) << 11;
        packed |= (to_int(z / 0.25f) & 0x3FF) << 22;
        *this << packed;
    }

    void appendPackGUID(U64 guid) {
        U8 packGUID[8 + 1];
        packGUID[0] = 0;
        size_t size = 1;
        for (U8 i = 0; guid != 0; ++i) {
            if (guid & 0xFF) {
                packGUID[0] |= static_cast<U8>(1 << i);
                packGUID[size] = static_cast<U8>(guid & 0xFF);
                ++size;
            }

            guid >>= 8;
        }

        append(packGUID, size);
    }

    void put(size_t pos, const U8 *src, size_t cnt) {
        if (pos + cnt > size())
            throw ByteBufferException(true, pos, cnt, size());
        memcpy(&_storage[pos], src, cnt);
    }

    void print_storage() const {}

    void textlike() const {}

    void hexlike() const {}

   private:
    /// limited for internal use because can "append" any unexpected type (like
    /// pointer and etc) with hard detection problem
    template <typename T>
    void append(T value) {
        EndianConvert(value);
        append((U8 *)&value, sizeof(value));
    }

   private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ACKNOWLEDGE_UNUSED(version);

        ar & _rpos;
        ar & _wpos;
        ar & _storage;
    }

   protected:
    size_t _rpos, _wpos;
    std::vector<U8> _storage;
};

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, Networking::vector3<T> const &v) {
    b << v._x;
    b << v._y;
    b << v._z;
    return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, Networking::vector3<T> &v) {
    b >> v._x;
    b >> v._y;
    b >> v._z;
    return b;
}

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, std::vector<T> const &v) {
    b << to_uint(v.size());
    for (const T& i  : v) {
        b << i;
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, std::vector<T> &v) {
    U32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--) {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, std::list<T> const &v) {
    b << to_uint(v.size());
    for (const T& i  : v) {
        b << i;
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, std::list<T> &v) {
    U32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--) {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer &operator<<(ByteBuffer &b, std::map<K, V> &m) {
    b << to_uint(m.size());
    for (typename std::map<K, V>::value_type i : m) {
        b << i.first;
        b << i.second;
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer &operator>>(ByteBuffer &b, std::map<K, V> &m) {
    U32 msize;
    b >> msize;
    m.clear();
    while (msize--) {
        K k;
        V v;
        b >> k >> v;
        m.insert(std::make_pair(k, v));
    }
    return b;
}

template <>
inline void ByteBuffer::read_skip<char *>() {
    stringImpl temp;
    *this >> temp;
}

template <>
inline void ByteBuffer::read_skip<char const *>() {
    read_skip<char *>();
}

template <>
inline void ByteBuffer::read_skip<stringImpl>() {
    read_skip<char *>();
}

};  // namespace Divide
#endif
