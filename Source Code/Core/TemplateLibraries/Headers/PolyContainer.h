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

struct PolyContainerEntry
{
    vec_size_eastl _typeIndex = 0;
    size_t _elementIndex = 0;

    template<typename T>
    inline T type() const { return static_cast<T>(_typeIndex); }
};

template<typename T>
using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

template<typename T, size_t N>
struct PolyContainer {
    template<typename U>
    inline typename std::enable_if<std::is_base_of<T, U>::value, PolyContainerEntry>::type
        insert(vec_size_eastl index, deleted_unique_ptr<T>&& cmd) {
        assert(index < N);

        _collection[index].push_back(std::move(cmd));

        return PolyContainerEntry{ index, _collection[index].size() - 1 };
    }

    inline T& get(vec_size_eastl index, size_t entry) {
        assert(index < N);
        assert(entry < _collection[index].size());

        return *_collection[index][entry];
    }

    inline const T& get(vec_size_eastl index, size_t entry) const {
        assert(index < N);
        assert(entry < _collection[index].size());

        return *_collection[index][entry];
    }

    inline T& get(const PolyContainerEntry& entry) {
        return get(entry._typeIndex, entry._elementIndex);
    }

    inline const T& get(const PolyContainerEntry& entry) const {
        return get(entry._typeIndex, entry._elementIndex);
    }


    inline vec_size_eastl size(vec_size_eastl index) {
        assert(index < N);

        return _collection[index].size();
    }

    inline void reserve(size_t size) {
        for (auto& col : _collection) {
            col.reserve(size);
        }
    }

    inline void reserve(vec_size_eastl index, size_t reserveSize) {
        assert(index < N);

        _collection[index].reserve(reserveSize);
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

    inline void clear(vec_size_eastl index, bool clearMemory = false) {
        assert(index < N);

        if (clearMemory) {
            _collection[index].clear();
        } else {
            _collection[index].resize(0);
        }
    }

    inline bool empty() const {
        for (auto col : _collection) {
            if (!col.empty()) {
                return false;
            }
        }

        return true;
    }

    std::array<vectorEASTL<deleted_unique_ptr<T>>, N> _collection;
};

#endif //_POLY_CONTAINER_H_