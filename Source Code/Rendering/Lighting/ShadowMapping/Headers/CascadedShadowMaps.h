/*
   Copyright (c) 2018 DIVIDE-Studio
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

#ifndef _C_SM_H_
#define _C_SM_H_

#include "ShadowMap.h"
#include "Platform/Video/Headers/PushConstants.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

class Quad3D;
class Camera;
class GFXDevice;
class ShaderBuffer;
class DirectionalLight;

struct DebugView;

FWD_DECLARE_MANAGED_CLASS(SceneGraphNode);

/// Directional lights can't deliver good quality shadows using a single shadow map.
/// This technique offers an implementation of the CSM method
class CascadedShadowMaps : public ShadowMap {
   public:
    explicit CascadedShadowMaps(GFXDevice& context, Light* light, const ShadowCameraPool& shadowCameras, U8 numSplits);
    ~CascadedShadowMaps();
    void render(U32 passIdx, GFX::CommandBuffer& bufferInOut);
    void postRender(GFX::CommandBuffer& bufferInOut);
    void init(ShadowMapInfo* const smi);

   protected:
    void calculateSplitDepths(const mat4<F32>& projMatrix, const vec2<F32>& nearFarPlanes);
    void applyFrustumSplits(const mat4<F32>& invViewMatrix, const vec2<F32>& nearFarPlanes);

   protected:
    U8 _numSplits;
    F32 _splitLogFactor;
    F32 _nearClipOffset;
    U32 _horizBlur;
    U32 _vertBlur;
    BoundingBox _previousFrustumBB;
    vec3<F32> _lightPosition;
    ShaderProgram_ptr _previewDepthMapShader;
    ShaderProgram_ptr _blurDepthMapShader;
    PushConstants     _blurDepthMapConstants;
    /// Shortcut for the owning directional light
    DirectionalLight* _dirLight;
    RenderTargetHandle _drawBuffer;
    RenderTargetHandle _blurBuffer;
    vector<vec3<F32> > _frustumCornersVS;
    vector<vec3<F32> > _frustumCornersWS;
    vector<vec3<F32> > _frustumCornersLS;
    vector<vec3<F32> > _splitFrustumCornersVS;
    vector<F32       > _splitDepths;
    std::array<mat4<F32>, Config::Lighting::MAX_SPLITS_PER_LIGHT> _shadowMatrices;
    std::array<DebugView*, Config::Lighting::MAX_SPLITS_PER_LIGHT> _debugViews;
};

};  // namespace Divide
#endif