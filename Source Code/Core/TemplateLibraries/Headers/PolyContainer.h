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
#pragma once
#ifndef _POLY_CONTAINER_H_
#define _POLY_CONTAINER_H_

namespace Divide {

#pragma pack(push, 1)
struct PolyContainerEntry
{
    static constexpr U32 INVALID_ENTRY_ID = std::numeric_limits<U32>::max();

    PolyContainerEntry() noexcept : PolyContainerEntry(0u, 0u) {}
    PolyContainerEntry(U8 typeIndex, U24 elementIndex) noexcept : _typeIndex(typeIndex), _elementIndex(elementIndex) {}
    PolyContainerEntry(const PolyContainerEntry& other) noexcept : _data(other._data) {}
    PolyContainerEntry(PolyContainerEntry&& other) noexcept : _data(std::move(other._data)) {}
    inline PolyContainerEntry& operator= (PolyContainerEntry&& other) noexcept {
        _data = std::move(other._data);
        return *this;
    }

    inline PolyContainerEntry& operator= (const PolyContainerEntry& other) noexcept {
        _data = other._data;
        return *this;
    }

    union {
        struct {
            U8 _typeIndex;
            U24 _elementIndex;
        };
        U32 _data = INVALID_ENTRY_ID;
    };
};
#pragma pack(pop)

constexpr bool VALIDATE_POLY_CONTAINERS = false;

template<typename T>
using PolyContainerDeleter = void (*)(T*&);

template<typename T>
using PolyContainerReserver = size_t(*)(U8);

template<typename T>
void DEFAULT_DEL(T*&) {}

template<typename T>
size_t DEFAULT_RES(U8) { return 0; }

template<typename T, U8 N, typename PolyContainerDeleter<T> DEL = DEFAULT_DEL<T>, typename PolyContainerReserver<T> RES = DEFAULT_RES<T>>
struct PolyContainer {
    using EntryList = vectorEASTLFast<T*>;

    PolyContainer()
    {
        for (U8 i = 0; i < N; ++i) {
            _collection[i].reserve(RES(i));
        }
    }

    ~PolyContainer()
    {
        clear(true);
    }

    template<typename U>
    inline typename std::enable_if<std::is_base_of<T, U>::value, PolyContainerEntry>::type
    insert(U8 index, T* cmd) {
        EntryList& collection = _collection[index];
        collection.emplace_back(cmd);

        return PolyContainerEntry{ index, to_U32(collection.size() - 1) };
    }

    template<typename U>
    inline typename std::enable_if<std::is_base_of<T, U>::value, T*>::type
    emplace(U8 index) {
        return _collection[index].emplace_back();
    }

    inline const EntryList& get(U8 index) const noexcept {
        return  _collection[index];
    }

    inline T* get(const PolyContainerEntry& entry) const noexcept {
        const EntryList& collection = _collection[entry._typeIndex];
        const U32 elementIndex = to_U32(entry._elementIndex);
        if (elementIndex < collection.size()) {
            return collection[elementIndex];
        }

        return nullptr;
    }

    inline bool exists(const PolyContainerEntry& entry) const noexcept {
        return entry._typeIndex < N && entry._elementIndex < _collection[entry._typeIndex].size();
    }

    inline void reserveAdditional(const PolyContainer<T,N, DEL, RES>& other) {
        for (U8 i = 0; i < N; ++i) {
            _collection[i].reserve(_collection[i].size() + other._collection[i].size());
        }
    }

    inline void clear() {
        for (U8 i = 0; i < N; ++i) {
            clear(i);
        }
    }

    inline void clear(U8 index) {
        EntryList& col = _collection[index];

        for (T*& cmd : col) {
            DEL(cmd);
        }
        col.clear();
    }

    inline bool empty() const noexcept {
        for (const EntryList& col : _collection) {
            if (!col.empty()) {
                return false;
            }
        }

        return true;
    }

    eastl::array<EntryList, N> _collection;
};

}; //namespace Divide
#endif //_POLY_CONTAINER_H_