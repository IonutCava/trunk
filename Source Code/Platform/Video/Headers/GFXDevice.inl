/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _HARDWARE_VIDEO_GFX_DEVICE_INL_H_
#define _HARDWARE_VIDEO_GFX_DEVICE_INL_H_

namespace Divide {

/// Render specified function inside of a viewport of specified dimensions and
/// position
inline void GFXDevice::renderInViewport(const vec4<I32>& rect,
                                        const DELEGATE_CBK<>& callback) {
    GFX::ScopedViewport viewport(rect);
    callback();
}

/// Compare the current render stage flag with the given mask
inline bool GFXDevice::isCurrentRenderStage(U32 renderStageMask) {
    DIVIDE_ASSERT(
        (renderStageMask & ~(to_uint(RenderStage::INVALID_STAGE) - 1)) == 0,
        "GFXDevice error: render stage query received an invalid bitmask!");
    return bitCompare(renderStageMask, to_uint(_renderStage));
}

inline bool GFXDevice::isCurrentRenderStage(RenderStage renderStage) {
        DIVIDE_ASSERT(renderStage != RenderStage::INVALID_STAGE,
        "GFXDevice error: render stage query received an invalid bitmask!");

    return renderStage == _renderStage;
}
/// Change the width of rendered lines to the specified value
inline void GFXDevice::setLineWidth(F32 width) {
    _previousLineWidth = _currentLineWidth;
    _currentLineWidth = width;
    _api->setLineWidth(width);
}
/// Restore the width of rendered lines to the previously set value
inline void GFXDevice::restoreLineWidth() { setLineWidth(_previousLineWidth); }
/// Query rasterization state
inline bool GFXDevice::rasterizationState() { return _rasterizationEnabled; }

/// Toggle hardware rasterization on or off.
inline void GFXDevice::toggleRasterization(bool state) {
    if (_rasterizationEnabled == state) {
        return;
    }
    _rasterizationEnabled = state;

    _api->toggleRasterization(state);
}
/// Register a function to be called in the 2D rendering fase of the GFX Flush
/// routine. Use callOrder for sorting purposes
inline void GFXDevice::add2DRenderFunction(const DELEGATE_CBK<>& callback,
                                           U32 callOrder) {
    _2dRenderQueue.push_back(std::make_pair(callOrder, callback));

    std::sort(std::begin(_2dRenderQueue), std::end(_2dRenderQueue),
              [](const std::pair<U32, DELEGATE_CBK<> >& a,
                 const std::pair<U32, DELEGATE_CBK<> >& b)
                  -> bool { return a.first < b.first; });
}

/// Sets the current render stage.
///@param stage Is used to inform the rendering pipeline what we are rendering.
/// Shadows? reflections? etc
inline RenderStage GFXDevice::setRenderStage(RenderStage stage) {
    std::swap(_renderStage, stage);
    return stage;
}
/// disable or enable a clip plane by index
inline void GFXDevice::toggleClipPlane(ClipPlaneIndex index, const bool state) {
    assert(index != ClipPlaneIndex::COUNT);
    U32 idx = to_uint(index);
    if (state != _clippingPlanes[idx].active()) {
        _clippingPlanes[idx].active(state);
        _api->updateClipPlanes();
    }
}
/// modify a single clip plane by index
inline void GFXDevice::setClipPlane(ClipPlaneIndex index, const Plane<F32>& p) {
    assert(index != ClipPlaneIndex::COUNT);
    _clippingPlanes[to_uint(index)] = p;
    updateClipPlanes();
}
/// set a new list of clipping planes. The old one is discarded
inline void GFXDevice::setClipPlanes(const PlaneList& clipPlanes) {
    if (clipPlanes != _clippingPlanes) {
        _clippingPlanes = clipPlanes;
        updateClipPlanes();
        _api->updateClipPlanes();
    }
}
/// clear all clipping planes
inline void GFXDevice::resetClipPlanes() {
    _clippingPlanes.resize(Config::MAX_CLIP_PLANES, Plane<F32>(0, 0, 0, 0));
    updateClipPlanes();
    _api->updateClipPlanes();
}
/// Alternative to the normal version of getMatrix
inline const mat4<F32>& GFXDevice::getMatrix(const MATRIX_MODE& mode) {
    getMatrix(mode, _mat4Cache);
    return _mat4Cache;
}
#define GFX_DEVICE GFXDevice::getInstance()
#define GFX_RENDER_BIN_SIZE \
    RenderPassManager::getInstance().getLastTotalBinSize(0)

};  // namespace Divide

#endif