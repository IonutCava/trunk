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

#ifndef _HARDWARE_VIDEO_GFX_DEVICE_INL_H_
#define _HARDWARE_VIDEO_GFX_DEVICE_INL_H_

namespace Divide {


inline void 
GFXDevice::NodeData::set(const GFXDevice::NodeData& other) {
    this->_worldMatrix.set(other._worldMatrix);
    this->_normalMatrixWV.set(other._normalMatrixWV);
    this->_colourMatrix.set(other._colourMatrix);
    this->_properties.set(other._properties);
}

inline const GFXShaderData::GPUData&
GFXDevice::renderingData() const {
    return _gpuBlock._data;
}

inline bool
GFXDevice::setViewport(I32 x, I32 y, I32 width, I32 height) {
    return setViewport(vec4<I32>(x, y, width, height));
}

inline bool 
GFXDevice::isDepthStage() const {
    return isDepthStage(getRenderStage());
}

inline bool 
GFXDevice::isDepthStage(const RenderStagePass& renderStagePass) const {
    return renderStagePass.stage() == RenderStage::SHADOW ||
           renderStagePass.pass() == RenderPassType::DEPTH_PASS;
}

/// Register a function to be called in the 2D rendering fase of the GFX Flush
/// routine. Use callOrder for sorting purposes
inline void 
GFXDevice::add2DRenderFunction(const GUID_DELEGATE_CBK& callback, U32 callOrder) {
    WriteLock w_lock(_2DRenderQueueLock);
    insert_sorted(_2dRenderQueue,
                  std::make_pair(callOrder, std::make_pair(callback.getGUID(), callback._callback)),
                  [](const std::pair<U32, GUID2DCbk >& a, const std::pair<U32, GUID2DCbk >& b) -> bool {
                      return a.first < b.first;
                  });
}

inline void
GFXDevice::remove2DRenderFunction(const GUID_DELEGATE_CBK& callback) {
    WriteLock w_lock(_2DRenderQueueLock);
    
    I64 targetGUID = callback.getGUID();
    _2dRenderQueue.erase(
        std::remove_if(std::begin(_2dRenderQueue), std::end(_2dRenderQueue),
            [&targetGUID](const std::pair<U32, GUID2DCbk >& callback)
            -> bool { return callback.second.first == targetGUID; }),
        std::end(_2dRenderQueue));
}

/// Sets the current render stage.
///@param stage Is used to inform the rendering pipeline what we are rendering.
/// Shadows? reflections? etc
inline const RenderStagePass& 
GFXDevice::setRenderStagePass(const RenderStagePass& stage) {
    _prevRenderStagePass = _renderStagePass;
    _renderStagePass = stage;
    return stage;
}
/// disable or enable a clip plane by index
inline void 
GFXDevice::toggleClipPlane(ClipPlaneIndex index, const bool state) {
    assert(index != ClipPlaneIndex::COUNT);
    U32 idx = to_U32(index);
    if (state != _clippingPlanes._active[idx]) {
        _clippingPlanes._active[idx] = state;
    }
}
/// modify a single clip plane by index
inline void 
GFXDevice::setClipPlane(ClipPlaneIndex index, const Plane<F32>& p, bool state) {
    assert(index != ClipPlaneIndex::COUNT);
    _clippingPlanes._planes[to_U32(index)] = p;
    _clippingPlanes._active[to_U32(index)] = state;
    updateClipPlanes();
}

/// set a new list of clipping planes. The old one is discarded
inline void 
GFXDevice::setClipPlanes(const ClipPlaneList& clipPlanes) {
    if (clipPlanes._active != _clippingPlanes._active ||
        clipPlanes._planes != _clippingPlanes._planes) {
        _clippingPlanes = clipPlanes;
        updateClipPlanes();
    }
}
/// clear all clipping planes
inline void 
GFXDevice::resetClipPlanes() {
    _clippingPlanes.resize(to_base(ClipPlaneIndex::COUNT), Plane<F32>(0, 0, 0, 0));
    updateClipPlanes();
}
/// Alternative to the normal version of getMatrix
inline const mat4<F32>& GFXDevice::getMatrix(const MATRIX& mode) const {
    return getMatrixInternal(mode);
}

};  // namespace Divide

#endif
