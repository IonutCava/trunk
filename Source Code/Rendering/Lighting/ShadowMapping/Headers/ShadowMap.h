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

#include "core.h"

enum ShadowType{
    SHADOW_TYPE_NONE = -1,
    SHADOW_TYPE_Single,
    SHADOW_TYPE_PSSM,
    SHADOW_TYPE_CubeMap,
    SHADOW_TYPE_PLACEHOLDER,
};

class Light;
class ParamHandler;
class FrameBufferObject;
class SceneRenderState;
///All the information needed for a single light's shadowmap
class ShadowMap {
public:
    ShadowMap(Light* light, ShadowType type);
    virtual ~ShadowMap();
    ///Render the scene and save the frame to the shadow map
    virtual void render(const SceneRenderState& renderState, boost::function0<void> sceneRenderFunction) = 0;
    ///Setup needed before rendering the light
    void preRender();
    ///Setup needed after rendering the light
    virtual void postRender();
    ///Get the current shadow mapping tehnique
    inline ShadowType getShadowMapType() const {return _shadowMapType;};

    inline  FrameBufferObject* getDepthMap() {return _depthMap;}
    inline  bool isBound() {return _isBound;}
            U16  resolution();
    virtual void resolution(U16 resolution, const SceneRenderState& sceneRenderState) {_init = true;}
    virtual bool Bind(U8 offset);
    virtual bool Unbind(U8 offset);
    virtual void previewShadowMaps() = 0;

protected:
    virtual void renderInternal(const SceneRenderState& renderState) const = 0;

protected:
    ShadowType _shadowMapType;
    ///The depth maps. Number depends on the current method
    FrameBufferObject*  _depthMap;
    ///A global resolution factor for all methods (higher = better quality)
    F32 _resolutionFactor;
    U16 _maxResolution;
    boost::function0<void> _callback;
    ///Internal pointer to the parrent light
    Light* _light;
    ParamHandler& _par;
    bool _init;
    bool _isBound;
};

class ShadowMapInfo {
public:
    ShadowMapInfo(Light* light);
    virtual ~ShadowMapInfo();
    inline ShadowMap* getShadowMap() {return _shadowMap;}
           ShadowMap* getOrCreateShadowMap(const SceneRenderState& sceneRenderState);
    inline U16  resolution() const {return _resolution;}

    inline void resolution(U16 resolution, const SceneRenderState& sceneRenderState) {
        _resolution = resolution;
        if(_shadowMap)
            _shadowMap->resolution(_resolution,sceneRenderState);
    }

private:
    U16        _resolution;
    ShadowMap* _shadowMap;
    Light*     _light;
public:

    U8         _numSplits;
};

#endif