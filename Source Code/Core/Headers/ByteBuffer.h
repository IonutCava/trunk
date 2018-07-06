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

#ifndef _CORE_BYTE_BUFFER_H_
#define _CORE_BYTE_BUFFER_H_

#include "Platform/Headers/ByteConverter.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>

namespace Divide {

class ByteBufferException {
   public:
    ByteBufferException(bool add, size_t pos, size_t esize, size_t size);
    void printPosError() const;

   private:
    bool   _add;
    size_t _pos;
    size_t _esize;
    size_t _size;
};

template <typename T>
struct Unused {
    Unused()
    {
    }
};

class ByteBuffer {
   public:
    const static U8 BUFFER_FORMAT_VERSION = 10;
    const static size_t DEFAULT_SIZE = 0x1000;

    // constructor
    ByteBuffer();

    // constructor
    ByteBuffer(size_t res);

    // copy constructor
    ByteBuffer(const ByteBuffer &buf);

    ByteBuffer& operator=(const ByteBuffer &buf);

    void clear();

    template <typename T>
    void put(size_t pos, const T& value);

    template <typename T>
    ByteBuffer& operator<<(const T& value);
    template<>
    ByteBuffer& operator<<(const bool& value);
    template <> 
    ByteBuffer& operator<<(const stringImpl& value);

    template <typename T>
    ByteBuffer& operator>>(T& value);
    template<>
    ByteBuffer& operator>>(bool& value);
    template<>
    ByteBuffer& operator>>(stringImpl& value);
    template <typename T>
    ByteBuffer& operator>>(const Unused<T>& value);

    template <typename T>
    void read_skip();
    void read_skip(size_t skip);

    template <typename T>
    T read();
    template <typename T>
    T read(size_t pos) const;
    void read(Byte *dest, size_t len);
    void readPackXYZ(F32& x, F32& y, F32& z);
    U64  readPackGUID();

    template <typename T>
    void append(const T *src, size_t cnt);
    void append(const Byte *src, size_t cnt);
    void append(const stringImpl& str);
    void append(const ByteBuffer &buffer);
    void appendPackXYZ(F32 x, F32 y, F32 z);
    void appendPackGUID(U64 guid);

    Byte operator[](size_t pos) const;
    size_t rpos() const;
    size_t rpos(size_t rpos_);
    size_t wpos() const;
    size_t wpos(size_t wpos_);
    size_t size() const;
    bool empty() const;
    void resize(size_t newsize);
    void reserve(size_t ressize);

    const Byte* contents() const;

    void put(size_t pos, const Byte *src, size_t cnt);

    bool dumpToFile(const stringImpl& fileName);
    bool loadFromFile(const stringImpl& fileName);

   private:
    /// limited for internal use because can "append" any unexpected type (like
    /// pointer and etc) with hard detection problem
    template <typename T>
    void append(const T& value);

   private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive &ar, const unsigned int version);

   protected:
    size_t _rpos, _wpos;
    vectorImpl<Byte> _storage;
};

};  // namespace Divide
#endif //_CORE_BYTE_BUFFER_H_

#include "ByteBuffer.inl"
