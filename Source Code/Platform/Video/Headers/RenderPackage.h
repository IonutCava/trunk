/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _RENDER_PACKAGE_H_
#define _RENDER_PACKAGE_H_

#include "CommandBuffer.h"

namespace Divide {

class RenderPackage {
public:
    RenderPackage() 
        : _isRenderable(false),
          _isOcclusionCullable(true)
    {
    }

    void clear();
    void set(const RenderPackage& other);

    inline void isRenderable(bool state) { _isRenderable = state; }
    inline bool isRenderable() const { return  _isRenderable; }

    inline void isOcclusionCullable(bool state) { _isOcclusionCullable = state; }
    inline bool isOcclusionCullable() const { return  _isOcclusionCullable; }

    size_t getSortKeyHash() const;

    GFX::CommandBuffer _commands;

private:
    bool _isRenderable;
    bool _isOcclusionCullable;
};

struct RenderPackageQueue {
public:
    RenderPackageQueue()
        : _locked(false),
          _currentCount(0)
    {
    }

    void clear();
    U32 size() const;
    bool empty() const;
    bool locked() const;

    const RenderPackage& getPackage(U32 idx) const;

protected:
    friend class GFXDevice;
    RenderPackage& getPackage(U32 idx);
    RenderPackage& back();
    bool push_back(const RenderPackage& package);
    void reserve(U16 size);
    void lock();
    void unlock();

protected:
    bool _locked;
    U32 _currentCount;
    vectorImpl<RenderPackage> _packages;
};

}; //namespace Divide

#endif //_RENDER_PACKAGE_H_
