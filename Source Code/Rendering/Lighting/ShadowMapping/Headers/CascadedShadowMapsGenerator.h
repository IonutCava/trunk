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

#pragma once
#ifndef _CSM_GENERATOR_H_
#define _CSM_GENERATOR_H_

#include "ShadowMap.h"
#include "Platform/Video/Headers/PushConstants.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

class Quad3D;
class Camera;
class Pipeline;
class GFXDevice;
class ShaderBuffer;
class DirectionalLightComponent;

struct DebugView;

FWD_DECLARE_MANAGED_CLASS(SceneGraphNode);

/// Directional lights can't deliver good quality shadows using a single shadow map.
/// This technique offers an implementation of the CSM method
class CascadedShadowMapsGenerator : public ShadowMapGenerator {
   public:
    explicit CascadedShadowMapsGenerator(GFXDevice& context);
    ~CascadedShadowMapsGenerator();

    void render(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) override;

   protected:
    using SplitDepths = std::array<F32, Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT>;

    void postRender(const DirectionalLightComponent& light, GFX::CommandBuffer& bufferInOut);
    SplitDepths calculateSplitDepths(const mat4<F32>& projMatrix,
                                     DirectionalLightComponent& light,
                                     const vec2<F32>& nearFarPlanes) noexcept;
    void applyFrustumSplits(DirectionalLightComponent& light,
                            const mat4<F32>& viewMatrix,
                            const mat4<F32>& projectionMatrix,
                            const vec2<F32>& nearFarPlanes,
                            U8 numSplits,
                            const SplitDepths& splitDepths);

    bool useMSAA() const noexcept;

  protected:
    Pipeline* _vertBlurPipeline = nullptr;
    Pipeline* _horzBlurPipeline = nullptr;
    Pipeline* _computeVSMPipeline[2] = { nullptr, nullptr };

    ShaderProgram_ptr _blurDepthMapShader = nullptr;
    ShaderProgram_ptr _computeVSMShader[2] = { nullptr, nullptr };
    PushConstants     _shaderConstants;
    RenderTargetHandle _drawBufferDepth;
    RenderTargetHandle _drawBufferResolve;
    RenderTargetHandle _blurBuffer;
};

};  // namespace Divide
#endif //_CSM_GENERATOR_H_