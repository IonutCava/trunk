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

#ifndef _SHADOW_MAP_H_
#define _SHADOW_MAP_H_

#include "Hardware/Video/Buffers/FrameBuffer/Headers/FrameBuffer.h"

enum ShadowType{
    SHADOW_TYPE_NONE = -1,
    SHADOW_TYPE_Single,
    SHADOW_TYPE_CSM,
    SHADOW_TYPE_CubeMap,
    SHADOW_TYPE_PLACEHOLDER,
};

class Light;
class ParamHandler;
class ShadowMapInfo;
class SceneRenderState;

///All the information needed for a single light's shadowmap
class ShadowMap {
public:
    ShadowMap(Light* light, ShadowType type);
    virtual ~ShadowMap();
    ///Render the scene and save the frame to the shadow map
    virtual void render(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction) = 0;
    ///Setup needed before rendering the light
    void preRender();
    ///Setup needed after rendering the light
    virtual void postRender();
    ///Get the current shadow mapping tehnique
    inline  ShadowType             getShadowMapType()     const { return _shadowMapType; }
    inline  FrameBuffer*           getDepthMap()                { return _depthMap; }
    inline  const vectorImpl<F32>& getShadowFloatValues() const { return _shadowFloatValues; }

    inline  bool isBound() {return _isBound;}

            U16  resolution();
    virtual void resolution(U16 resolution, U8 resolutionFactor) {}

    virtual void init(ShadowMapInfo* const smi) = 0;
    virtual bool Bind(U8 offset);
    virtual bool Unbind(U8 offset);
    virtual void previewShadowMaps() = 0;
    virtual void togglePreviewShadowMaps(bool state) {}
    virtual void updateResolution(I32 newWidth, I32 newHeight) {}

protected:
    ShadowType _shadowMapType;
    ///The depth maps. Number depends on the current method
    FrameBuffer* _depthMap;
    U16 _resolution;
    ///Internal pointer to the parent light
    Light* _light;
    ParamHandler& _par;
    bool _init;
    bool _isBound;
    mat4<F32> _bias;
    // _shadowFloatValues are generic floating point values needed for shadow mapping such as farBounds, biases, etc.
    vectorImpl<F32>        _shadowFloatValues;
};

class ShadowMapInfo {
public:
    ShadowMapInfo(Light* light);
    virtual ~ShadowMapInfo();
    inline ShadowMap* getShadowMap() {return _shadowMap;}
           ShadowMap* getOrCreateShadowMap(const SceneRenderState& sceneRenderState);
    inline U16  resolution()       const {return _resolution;}
           void resolution(U16 resolution);

    inline U8    numLayers()              const {return _numLayers;}
    inline  void numLayers(U8 layerCount)       { _numLayers = std::min(std::abs(layerCount), Config::MAX_SPLITS_PER_LIGHT); }

private:
    U8         _numLayers;
    U16        _resolution;
    ShadowMap* _shadowMap;
    Light*     _light;
};

#endif