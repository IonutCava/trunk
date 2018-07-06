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

/// add a new clipping plane. This will be limited by the actual shaders (how many planes they use)
/// this function returns the newly added clip plane's index in the vector
inline I32 GFXDevice::addClipPlane(const Plane<F32>& p){
    if(_clippingPlanes.size() == Config::MAX_CLIP_PLANES){
        //overwrite the clipping planes from the front
        _clippingPlanes.erase(_clippingPlanes.begin());
    }
    _clippingPlanes.push_back(p);
    _clippingPlanesDirty = true;
    return (I32)(_clippingPlanes.size() - 1);
}

/// add a new clipping plane defined by it's equation's coefficients
inline I32 GFXDevice::addClipPlane(F32 A, F32 B, F32 C, F32 D) {
    return addClipPlane(Plane<F32>(A, B, C, D));
}

/// remove a clip plane by index
inline bool GFXDevice::removeClipPlane(U32 index) {
    if(index < _clippingPlanes.size() && index >= 0) {
        _clippingPlanes.erase(_clippingPlanes.begin() + index);
        _clippingPlanesDirty = true;
        return true;
    }
    return false;
}

/// disable a clip plane by index
inline bool GFXDevice::disableClipPlane(U32 index) {
    if(index < _clippingPlanes.size() && index >= 0) {
        _clippingPlanes[index].active(false);
        _clippingPlanesDirty = true;
        return true;
    }
    return false;
}

/// enable a clip plane by index
inline bool GFXDevice::enableClipPlane(U32 index) {
    if(index < _clippingPlanes.size() && index >= 0) {
        _clippingPlanes[index].active(true);
        _clippingPlanesDirty = true;
        return true;
    }
    return false;
}

/// modify a single clip plane by index
inline void GFXDevice::setClipPlane(U32 index, const Plane<F32>& p){
    CLAMP<U32>(index, 0 ,(U32)_clippingPlanes.size());
    _clippingPlanes[index] = p;
    _clippingPlanesDirty = true;
}

/// set a new list of clipping planes. The old one is discarded
inline void GFXDevice::setClipPlanes(const PlaneList& clipPlanes)  {
    if (clipPlanes != _clippingPlanes) {
        _clippingPlanes = clipPlanes;
        _clippingPlanesDirty = true;
    }
}

/// clear all clipping planes
inline void GFXDevice::resetClipPlanes() {
    if (!_clippingPlanes.empty()) {
        _clippingPlanes.clear();
        _clippingPlanesDirty = true;
    }
}

#define GFX_DEVICE GFXDevice::getInstance()
#define GFX_RENDER_BIN_SIZE RenderPassManager::getInstance().getLastTotalBinSize(0)

inline RenderStateBlock* SET_STATE_BLOCK(RenderStateBlock* const block, bool forceUpdate = false){
    return GFX_DEVICE.setStateBlock(block,forceUpdate);
}

inline RenderStateBlock* SET_DEFAULT_STATE_BLOCK(bool forceUpdate = false){
    return GFX_DEVICE.setDefaultStateBlock(forceUpdate);
}

#endif