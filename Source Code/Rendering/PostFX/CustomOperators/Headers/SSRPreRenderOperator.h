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
#ifndef _SSR_PRE_RENDER_OPERATOR_H_
#define _SSR_PRE_RENDER_OPERATOR_H_

#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/PostFX/Headers/PreRenderOperator.h"

namespace Divide {

class SSRPreRenderOperator final : public PreRenderOperator {
   public:
       struct Parameters {
           F32 _maxDistance = 100.f;   // Maximum camera-space distance to trace before returning a miss
           F32 _jitterAmount = 1.f;
           F32 _stride = 8.f;
           F32 _zThickness = 1.5f; // Camera space thickness to ascribe to each pixel in the depth buffer
           F32 _strideZCutoff = 100.f;
           F32 _screenEdgeFadeStart = 0.75f;
           F32 _eyeFadeStart = 0.5f;
           F32 _eyeFadeEnd = 1.f;
           U16 _maxSteps = 256;  // Maximum number of iterations. Higher gives better images but may be slow
           U8  _binarySearchIterations = 4u;
       };

   public:
    SSRPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache* cache);
    ~SSRPreRenderOperator() = default;

    [[nodiscard]] bool execute(const Camera* camera, const RenderTargetHandle& input, const RenderTargetHandle& output, GFX::CommandBuffer& bufferInOut) override;
    void reshape(U16 width, U16 height) override;

    PROPERTY_R(Parameters, parameters);

    void parameters(const Parameters& params) noexcept;

    [[nodiscard]] bool ready() const override;

   private:
     ShaderProgram_ptr _ssrShader = nullptr;
     Pipeline* _pipeline = nullptr;
     PushConstants _constants;
     mat4<F32> _projToPixelBasis;
     vec2<F32> _cachedZPlanes = VECTOR2_UNIT;
     bool _constantsDirty = true;
};

}  // namespace Divide

#endif //_SSR_PRE_RENDER_OPERATOR_H_