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

#ifndef _HARDWARE_VIDEO_GFX_DEVICE_INL_H_
#define _HARDWARE_VIDEO_GFX_DEVICE_INL_H_

namespace Divide {

#define GFX_DEVICE GFXDevice::instance()
#define GFX_RENDER_BIN_SIZE \
    RenderPassManager::instance().getLastTotalBinSize(RenderStage::DISPLAY)
#define GFX_HIZ_CULL_COUNT \
    GFX_DEVICE.getLastCullCount()

inline F32 GFXDevice::GPUBlock::GPUData::aspectRatio() const {
    return _cameraPosition.w;
}

inline vec2<F32> GFXDevice::GPUBlock::GPUData::currentZPlanes() const {
    return _ZPlanesCombined.xy();
}

inline F32 GFXDevice::GPUBlock::GPUData::FoV() const {
    return _renderProperties.z;
}

inline F32 GFXDevice::GPUBlock::GPUData::tanHFoV() const {
    return _renderProperties.w;
}

inline void 
GFXDevice::NodeData::set(const GFXDevice::NodeData& other) {
    this->_worldMatrix.set(other._worldMatrix);
    this->_normalMatrixWV.set(other._normalMatrixWV);
    this->_colorMatrix.set(other._colorMatrix);
    this->_properties.set(other._properties);
}

inline void 
GFXDevice::ShaderBufferBinding::set(const GFXDevice::ShaderBufferBinding& other) {
    set(other._slot, other._buffer, other._range);
}

inline void
GFXDevice::ShaderBufferBinding::set(ShaderBufferLocation slot,
                                    ShaderBuffer* buffer,
                                    const vec2<U32>& range) {
    _slot = slot;
    _buffer = buffer;
    _range.set(range);
}

inline void 
GFXDevice::RenderPackage::clear() {
    _shaderBuffers.resize(0);
    _textureData.resize(0);
    _drawCommands.resize(0);
}

inline void 
GFXDevice::RenderPackage::set(const GFXDevice::RenderPackage& other) {
#if 0
    size_t shaderBufferSize = other._shaderBuffers.size();
    size_t textureDataSize = other._textureData.size();
    size_t drawCommandSize = other._drawCommands.size();

    _shaderBuffers.resize(shaderBufferSize);
    _textureData.resize(textureDataSize);
    _drawCommands.resize(drawCommandSize);
    for (size_t i = 0; i < shaderBufferSize; ++i) {
        _shaderBuffers.at(i).set(other._shaderBuffers.at(i));
    }

    for (size_t i = 0; i < textureDataSize; ++i) {
        _textureData.at(i).set(other._textureData.at(i));
    }

    for (size_t i = 0; i < drawCommandSize; ++i) {
        _drawCommands.at(i).set(other._drawCommands.at(i));
    }
#else
    _shaderBuffers = other._shaderBuffers;
    _textureData = other._textureData;
    _drawCommands = other._drawCommands;
#endif
}

inline void 
GFXDevice::RenderQueue::clear() {
    for (U32 idx = 0; idx < _currentCount; ++idx) {
        //_packages[idx].clear();
    }
    _currentCount = 0;
}

inline U32 
GFXDevice::RenderQueue::size() const {
    return _currentCount;
}

inline bool
GFXDevice::RenderQueue::locked() const {
    return _locked;
}

inline bool 
GFXDevice::RenderQueue::empty() const {
    return _currentCount == 0;
}

inline const GFXDevice::RenderPackage& 
GFXDevice::RenderQueue::getPackage(U32 idx) const {
    assert(idx < Config::MAX_VISIBLE_NODES);
    return _packages[idx];
}

inline GFXDevice::RenderPackage&
GFXDevice::RenderQueue::getPackage(U32 idx) {
    return _packages.at(idx);
}

inline GFXDevice::RenderPackage&
GFXDevice::RenderQueue::back() {
    return _packages.at(std::max(to_int(_currentCount) - 1, 0));
}

inline bool
GFXDevice::RenderQueue::push_back(const RenderPackage& package) {
    if (_currentCount <= Config::MAX_VISIBLE_NODES) {
        _packages.at(_currentCount++).set(package);
        return true;
    }
    return false;
}

inline void
GFXDevice::RenderQueue::reserve(U16 size) {
    _packages.resize(size);
}

inline void
GFXDevice::RenderQueue::lock() {
    _locked = true;
}

inline void
GFXDevice::RenderQueue::unlock() {
    _locked = false;
}

inline void
GFXDevice::RenderTarget::cacheSettings() {
    _renderSettings._aspectRatio = GFX_DEVICE.renderingData().aspectRatio();
    _renderSettings._zPlanes.set(GFX_DEVICE.renderingData().currentZPlanes());
    GFX_DEVICE.getMatrix(MATRIX::PROJECTION, _renderSettings._projectionMatrix);
    GFX_DEVICE.getMatrix(MATRIX::VIEW, _renderSettings._viewMatrix);
}

inline const GFXDevice::GPUBlock::GPUData&
GFXDevice::renderingData() const {
    return _gpuBlock._data;
}

inline void
GFXDevice::setViewport(I32 x, I32 y, I32 width, I32 height) {
    setViewport(vec4<I32>(x, y, width, height));
}

inline bool 
GFXDevice::isDepthStage() const {
    return getRenderStage() == RenderStage::SHADOW ||
           getRenderStage() == RenderStage::Z_PRE_PASS;
}

/// Query rasterization state
inline bool 
GFXDevice::rasterizationState() { 
    return _rasterizationEnabled; 
}

/// Toggle writes to the depth buffer on or off
inline void 
GFXDevice::toggleDepthWrites(bool state) {
    if(_zWriteEnabled == state) {
        return;
    }
    _zWriteEnabled = state;

    _api->toggleDepthWrites(state);
}


/// Toggle hardware rasterization on or off.
inline void 
GFXDevice::toggleRasterization(bool state) {
    if (_rasterizationEnabled == state) {
        return;
    }
    _rasterizationEnabled = state;

    _api->toggleRasterization(state);
}
/// Register a function to be called in the 2D rendering fase of the GFX Flush
/// routine. Use callOrder for sorting purposes
inline void 
GFXDevice::add2DRenderFunction(const DELEGATE_CBK<>& callback, U32 callOrder) {
    _2dRenderQueue.push_back(std::make_pair(callOrder, callback));

    std::sort(std::begin(_2dRenderQueue), std::end(_2dRenderQueue),
              [](const std::pair<U32, DELEGATE_CBK<> >& a,
                 const std::pair<U32, DELEGATE_CBK<> >& b)
                  -> bool { return a.first < b.first; });
}

/// Sets the current render stage.
///@param stage Is used to inform the rendering pipeline what we are rendering.
/// Shadows? reflections? etc
inline RenderStage 
GFXDevice::setRenderStage(RenderStage stage) {
    _prevRenderStage = _renderStage;
    _renderStage = stage;
    return stage;
}
/// disable or enable a clip plane by index
inline void 
GFXDevice::toggleClipPlane(ClipPlaneIndex index, const bool state) {
    assert(index != ClipPlaneIndex::COUNT);
    U32 idx = to_uint(index);
    if (state != _clippingPlanes[idx].active()) {
        _clippingPlanes[idx].active(state);
        _api->updateClipPlanes();
    }
}
/// modify a single clip plane by index
inline void 
GFXDevice::setClipPlane(ClipPlaneIndex index, const Plane<F32>& p) {
    assert(index != ClipPlaneIndex::COUNT);
    _clippingPlanes[to_uint(index)] = p;
    updateClipPlanes();
}
/// set a new list of clipping planes. The old one is discarded
inline void 
GFXDevice::setClipPlanes(const PlaneList& clipPlanes) {
    if (clipPlanes != _clippingPlanes) {
        _clippingPlanes = clipPlanes;
        updateClipPlanes();
        _api->updateClipPlanes();
    }
}
/// clear all clipping planes
inline void 
GFXDevice::resetClipPlanes() {
    _clippingPlanes.resize(Config::MAX_CLIP_PLANES, Plane<F32>(0, 0, 0, 0));
    updateClipPlanes();
    _api->updateClipPlanes();
}
/// Alternative to the normal version of getMatrix
inline const mat4<F32>& 
GFXDevice::getMatrix(const MATRIX& mode) {
    getMatrix(mode, _mat4Cache);
    return _mat4Cache;
}

inline ShaderBuffer& GFXDevice::getCommandBuffer(RenderStage stage, U32 pass) const {
    U32 bufferIdx = getNodeBufferIndexForStage(stage);
    assert(pass < MAX_PASSES_PER_STAGE && _indirectCommandBuffers[bufferIdx][pass]);
    return *_indirectCommandBuffers[bufferIdx][pass].get();
}

inline U32 GFXDevice::getNodeBufferIndexForStage(RenderStage stage) const {
    U32 bufferIdx = 0;
    switch (stage) {
    case RenderStage::REFLECTION:
        bufferIdx = 1;
        break;
    case RenderStage::SHADOW:
        bufferIdx = 2;
        break;
    };

    return bufferIdx;
}

inline ShaderBuffer& GFXDevice::getNodeBuffer(RenderStage stage, U32 pass) const {
    U32 bufferIdx = getNodeBufferIndexForStage(stage);
    assert(pass < MAX_PASSES_PER_STAGE && _nodeBuffers[bufferIdx][pass]);
    return *_nodeBuffers[bufferIdx][pass].get();
}

/// Submit multiple draw commands that use the same source buffer (e.g. terrain or batched meshes)
inline void
GFXDevice::submitCommands(const vectorImpl<GenericDrawCommand>& cmds, bool useIndirectRender) {
    for (const GenericDrawCommand& cmd : cmds) {
        // Data validation is handled in the single command version
        submitCommand(cmd, useIndirectRender);
    }
}

};  // namespace Divide

#endif
