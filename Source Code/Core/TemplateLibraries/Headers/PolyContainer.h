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
        I32 _data = 0;
    };
};
#pragma pack(pop)

template<typename T>
using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T*&)>>;

constexpr bool VALIDATE_POLY_CONTAINERS = false;

template<typename T, U8 N>
struct PolyContainer {

    using EntryList = vectorEASTLFast<deleted_unique_ptr<T>>;

    template<typename U>
    inline typename std::enable_if<std::is_base_of<T, U>::value, PolyContainerEntry>::type
        insert(U8 index, deleted_unique_ptr<T>&& cmd) {
        if (VALIDATE_POLY_CONTAINERS) {
            assert(index < N);
        }

        EntryList& collection = _collection[index];
        collection.push_back(std::move(cmd));

        return PolyContainerEntry{ index, to_I32(collection.size() - 1) };
    }

    inline EntryList& get(U8 index) noexcept {
        if (VALIDATE_POLY_CONTAINERS) {
            assert(index < N);
        }

        return  _collection[index];
    }

    inline const EntryList& get(U8 index) const noexcept {
        if (VALIDATE_POLY_CONTAINERS) {
            assert(index < N);
        }

        return  _collection[index];
    }

    inline T* getPtr(const PolyContainerEntry& entry) noexcept {
        return getPtr(entry._typeIndex, entry.elementIndex());
    }

    inline const T* getPtr(const PolyContainerEntry& entry) const noexcept {
        return getPtr(entry._typeIndex, entry.elementIndex());
    }

    inline T* getPtr(U8 index, I32 entry) const noexcept {
        const EntryList& collection = get(index);
        return collection[entry].get();
    }

    inline T* getPtrOrNull(U8 index, I32 entry) const noexcept {
        const EntryList& collection = get(index);
        assert(entry >= 0);
        if (entry < collection.size()) {
            return collection[entry].get();
        }

        return nullptr;
    }

    inline T& get(U8 index, I32 entry) noexcept {
        return *getPtr(index, entry);
    }

    inline const T& get(U8 index, I32 entry) const noexcept {
        return *getPtr(index, entry);
    }

    inline T& get(const PolyContainerEntry& entry) noexcept {
        return *getPtr(entry._typeIndex, entry.elementIndex());
    }

    inline const T& get(const PolyContainerEntry& entry) const noexcept {
        return *getPtr(entry._typeIndex, entry.elementIndex());
    }

    inline bool exists(const PolyContainerEntry& entry) const noexcept {
        return exists(entry._typeIndex, entry.elementIndex());
    }

    inline bool exists(U8 index, I32 entry) const noexcept{
        return index < N && entry < size(index);
    }

    inline vec_size_eastl size(U8 index) const noexcept{
        return get(index).size();
    }

    inline void reserve(size_t size) {
        for (auto& col : _collection) {
            col.reserve(size);
        }
    }

    inline void reserve(U8 index, size_t reserveSize) {
        getPtr(index)->reserve(reserveSize);
    }

    inline void clear(bool clearMemory = false) {
        if (clearMemory) {
            for (auto& col : _collection) {
                col.clear();
            }
        } else {
            for (auto& col : _collection) {
                col.resize(0);
            }
        }
    }

    inline void nuke() {
        for (auto& col : _collection) {
            auto size = col.size();
            col.reset_lose_memory();
            col.reserve(size);
        }
    }

    inline void clear(U8 index, bool clearMemory = false) {
        if (clearMemory) {
            getPtr(index)->clear();
        } else {
            getPtr(index)->resize(0);
        }
    }

    inline bool empty() const noexcept {
        for (const auto& col : _collection) {
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