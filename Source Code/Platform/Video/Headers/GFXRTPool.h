/*
Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _HARDWARE_VIDEO_GFX_RT_POOL_H_
#define _HARDWARE_VIDEO_GFX_RT_POOL_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

class RenderTarget;

class GFXRTPool {
protected:
    friend class GFXDevice;
    GFXRTPool();
    ~GFXRTPool();

    inline RenderTarget& renderTarget(const RenderTargetHandle& handle) {
        return renderTarget(handle._targetID);
    }

    inline const RenderTarget& renderTarget(const RenderTargetHandle& handle) const {
        return renderTarget(handle._targetID);
    }

    inline RenderTarget& renderTarget(RenderTargetID target) {
        return *_renderTargets[to_uint(target._usage)][target._index];
    }

    inline const RenderTarget& renderTarget(RenderTargetID target) const {
        return *_renderTargets[to_uint(target._usage)][target._index];
    }

    inline vectorImpl<RenderTarget*>& renderTargets(RenderTargetUsage target) {
        return _renderTargets[to_uint(target)];
    }

    inline void set(const RenderTargetHandle& handle, RenderTarget* newTarget) {
        set(handle._targetID, newTarget);
    }

    void clear();
    void set(RenderTargetID target, RenderTarget* newTarget);
    RenderTargetHandle add(RenderTargetUsage targetUsage, RenderTarget* newTarget);
    bool remove(RenderTargetHandle& handle);

protected:
    std::array<vectorImpl<RenderTarget*>, to_const_uint(RenderTargetUsage::COUNT)> _renderTargets;
};
}; //namespace Divide

#endif //_HARDWARE_VIDEO_GFX_RT_POOL_H_