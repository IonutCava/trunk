/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _VDI_POOL_H_
#define _VDI_POOL_H_

#include "Platform/Headers/PlatformDataTypes.h"
#include "Platform/Threading/Headers/SharedMutex.h"
#include "Core/TemplateLibraries/Headers/Vector.h"

namespace Divide {

class VertexDataInterface;

#pragma pack(push, 1)
struct VDIHandle {
    U16 _id = 0;
    U8  _generation = 0;

    inline bool operator== (const VDIHandle& val) const {
        return _id == val._id && _generation == val._generation;
    }

    inline bool operator!= (const VDIHandle& val) const {
        return _id != val._id || _generation != val._generation;
    }
};
#pragma pack(pop)

template<size_t N>
class VDIPool {
public:
    VDIPool()
    {
        _ids.fill({ 0u });
        _pool.fill(nullptr);
    }

    VDIHandle allocate(VertexDataInterface& vdi);
    void deallocate(VDIHandle handle);

    VertexDataInterface* find(VDIHandle handle) const;

protected:
    mutable SharedMutex _poolLock;
    std::array<VDIHandle, N> _ids;
    std::array<VertexDataInterface*, N> _pool;
};

}; //namespace Divide

#endif //_VDI_POOL_H_

#include "VDIPool.inl"