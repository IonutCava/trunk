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
#ifndef _POLY_CONTAINER_H_
#define _POLY_CONTAINER_H_

namespace Divide {

#pragma pack(push, 1)
struct PolyContainerEntry
{
    static constexpr I32 INVALID_ENTRY_ID = std::numeric_limits<I32>::min();

    PolyContainerEntry() : _data(0) {}
    PolyContainerEntry(U8 type, I24 element) : _typeIndex(type), _elementIndex(element) {}
    PolyContainerEntry(const PolyContainerEntry& other) : _data(other._data) {}
    PolyContainerEntry(PolyContainerEntry&& other) noexcept : _data(std::move(other._data)) {}
    

    PolyContainerEntry& operator=(const PolyContainerEntry& other) {
        _data = other._data;
        return *this;
    }

    PolyContainerEntry& operator=(PolyContainerEntry&& other) noexcept {
        _data = std::move(other._data);
        return *this;
    }
    
    ~PolyContainerEntry() = default;

    FORCE_INLINE const I32 elementIndex() const noexcept { return to_I32(_elementIndex); }

    union {
        struct {
            U8 _typeIndex;
            I24 _elementIndex;
        };
        I32 _data = INVALID_ENTRY_ID;
    };
};
#pragma pack(pop)

constexpr bool VALIDATE_POLY_CONTAINERS = false;

template<typename T>
using PolyContainerDeleter = void (*)(T*&);

template<typename T, U8 N, typename PolyContainerDeleter<T> DEL>
struct PolyContainer {
    using EntryList = vectorEASTLFast<T*>;

    ~PolyContainer()
    {
        clear(true);
    }

    template<typename U>
    inline typename std::enable_if<std::is_base_of<T, U>::value, PolyContainerEntry>::type
        insert(U8 index, T* cmd) {
        if (VALIDATE_POLY_CONTAINERS) {
            assert(index < N);
        }

        EntryList& collection = _collection[index];
        collection.emplace_back(cmd);

        return PolyContainerEntry{ index, to_I32(collection.size() - 1) };
    }

    template<typename U>
    inline typename std::enable_if<std::is_base_of<T, U>::value, T*>::type
        emplace(U8 index) {
        if (VALIDATE_POLY_CONTAINERS) {
            assert(index < N);
        }

        EntryList& collection = _collection[index];
        collection.emplace_back({});

        return collection.back();
    }

    inline const EntryList& get(U8 index) const noexcept {
        if (VALIDATE_POLY_CONTAINERS) {
            assert(index < N);
        }

        return  _collection[index];
    }

    FORCE_INLINE T* get(const PolyContainerEntry& entry) const noexcept {
        return get(entry._typeIndex, entry.elementIndex());
    }

    inline T* get(U8 index, I32 entry) const noexcept {
        const EntryList& collection = get(index);
        if (entry < collection.size()) {
            return collection[entry];
        }

        return nullptr;
    }

    inline bool exists(const PolyContainerEntry& entry) const noexcept {
        return exists(entry._typeIndex, entry.elementIndex());
    }

    inline bool exists(U8 index, I32 entry) const noexcept{
        return index < N && entry < _collection[index].size();
    }

    inline void reserve(size_t size) {
        for (EntryList& col : _collection) {
            col.reserve(size);
        }
    }

    inline void reserveAdditional(const PolyContainer<T,N, DEL>& other) {
        for (U8 i = 0; i < N; ++i) {
            _collection[i].reserve(_collection[i].size() + other._collection[i].size());
        }
    }

    inline void reserve(U8 index, size_t reserveSize) {
        get(index).reserve(reserveSize);
    }

    inline void clear(bool clearMemory = false) {
        for (U8 i = 0; i < N; ++i) {
            clear(i, clearMemory);
        }
    }

    inline void clear(U8 index, bool clearMemory = false) {
        EntryList& col = _collection[index];

        for (T*& cmd : col) {
            DEL(cmd);
        }

        if (clearMemory) {
            col.clear();
        } else {
            col.resize(0);
        }
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