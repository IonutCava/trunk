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

#ifndef _SHADOW_MAP_H_
#define _SHADOW_MAP_H_

#include "config.h"

#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

enum class ShadowType : U32 {
    SINGLE = 0,
    LAYERED,
    CUBEMAP,
    COUNT
};

class Light;
class Camera;
class ShadowMapInfo;
class SceneRenderState;

typedef std::array<Camera*, Config::Lighting::MAX_SPLITS_PER_LIGHT> ShadowCameraPool;

/// All the information needed for a single light's shadowmap
class NOINITVTABLE ShadowMap {
   public:
    explicit ShadowMap(GFXDevice& context, Light* light, const ShadowCameraPool& shadowCameras, ShadowType type);
    virtual ~ShadowMap();

    /// Render the scene and save the frame to the shadow map
    virtual void render(GFXDevice& context, U32 passIdx) = 0;
    /// Get the current shadow mapping tehnique
    inline ShadowType getShadowMapType() const { return _shadowMapType; }

    RenderTarget& getDepthMap();
    const RenderTarget& getDepthMap() const;

    RenderTargetID getDepthMapID();
    const RenderTargetID getDepthMapID() const;

    inline U32 getArrayOffset() const {
        return _arrayOffset;
    }

    virtual void init(ShadowMapInfo* const smi) = 0;
    static void resetShadowMaps(GFXDevice& context);
    static void initShadowMaps(GFXDevice& context);
    static void clearShadowMaps(GFXDevice& context);
    static void bindShadowMaps(GFXDevice& context);
    static U16  findDepthMapLayer(ShadowType shadowType);
    static void commitDepthMapLayer(ShadowType shadowType, U32 layer);
    static bool freeDepthMapLayer(ShadowType shadowType, U32 layer);
    static void clearShadowMapBuffers(GFXDevice& context);

   protected:
    GFXDevice& _context;

    typedef std::array<bool, Config::Lighting::MAX_SHADOW_CASTING_LIGHTS> LayerUsageMask;
    static std::array<LayerUsageMask, to_base(ShadowType::COUNT)> _depthMapUsage;
    
    ShadowType _shadowMapType;
    U16 _arrayOffset;
    /// Internal pointer to the parent light
    Light* _light;
    const ShadowCameraPool& _shadowCameras;
    bool _init;
};

class ShadowMapInfo {
   public:
    ShadowMapInfo(Light* light);
    virtual ~ShadowMapInfo();

    inline ShadowMap* getShadowMap() { return _shadowMap; }

    ShadowMap* createShadowMap(GFXDevice& context, const SceneRenderState& sceneRenderState, const ShadowCameraPool& shadowCameras);

    inline U8 numLayers() const { return _numLayers; }

    inline void numLayers(U8 layerCount) {
        _numLayers = std::min(layerCount, to_U8(Config::Lighting::MAX_SPLITS_PER_LIGHT));
    }

   private:
    U8 _numLayers;
    ShadowMap* _shadowMap;
    Light* _light;
};

};  // namespace Divide

#endif