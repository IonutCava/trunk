/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _HARDWARE_VIDEO_GFX_DEVICE_INL_H_
#define _HARDWARE_VIDEO_GFX_DEVICE_INL_H_

inline void GFXDevice::setApi(const RenderAPI& api){
    _api.setId(api);
    switch (api)	{
        default:
        case OpenGLES:
        case OpenGL:    _api = GL_API::getOrCreateInstance();	break;
        case Direct3D:	_api = DX_API::getOrCreateInstance();	break;

        case GFX_RENDER_API_PLACEHOLDER:
        case None:	ERROR_FN(Locale::get("ERROR_GFX_DEVICE_API")); setApi(OpenGL); break; 
    };
}

/// Render specified function inside of a viewport of specified dimensions and position
inline void GFXDevice::renderInViewport(const vec4<I32>& rect, const DELEGATE_CBK& callback){
    setViewport(rect);
    callback();
    restoreViewport();
}

/// Compare the current render stage flag with the given mask
inline bool GFXDevice::isCurrentRenderStage(U8 renderStageMask) {
    DIVIDE_ASSERT((renderStageMask & ~(INVALID_STAGE - 1)) == 0, "GFXDevice error: render stage query received an invalid bitmask!"); 
    return bitCompare(renderStageMask, _renderStage);
}

/// Toggle hardware rasterization on or off. 
inline void GFXDevice::toggleRasterization(bool state) {
    if (_rasterizationEnabled == state) return;
    _rasterizationEnabled = state;

    _api.toggleRasterization(state);
}

/// Register a function to be called in the 2D rendering fase of the GFX Flush routine. Use callOrder for sorting purposes 
inline void GFXDevice::add2DRenderFunction(const DELEGATE_CBK& callback, U32 callOrder) {
    _2dRenderQueue.push_back(std::make_pair(callOrder, callback));

    std::sort(_2dRenderQueue.begin(), 
              _2dRenderQueue.end(), 
              [](const std::pair<U32, DELEGATE_CBK> & a, const std::pair<U32, DELEGATE_CBK> & b) -> bool {
                    return a.first < b.first;
              });
}

#define GFX_DEVICE GFXDevice::getInstance()
#define GFX_RENDER_BIN_SIZE RenderPassManager::getInstance().getLastTotalBinSize(0)

#endif