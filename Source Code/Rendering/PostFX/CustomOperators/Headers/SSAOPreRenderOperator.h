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

#ifndef _SSAO_PRE_RENDER_OPERATOR_H_
#define _SSAO_PRE_RENDER_OPERATOR_H_

#include "Rendering/PostFX/Headers/PreRenderOperator.h"

namespace Divide {

class SSAOPreRenderOperator final : public PreRenderOperator {
   public:
    SSAOPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache* cache);
    ~SSAOPreRenderOperator();

    void prepare(const Camera& camera, GFX::CommandBuffer& bufferInOut) final;
    void execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) final;
    void reshape(U16 width, U16 height) final;

    inline F32 radius() const { return _radius; }
    void radius(const F32 val);

    inline F32 power() const { return _power; }
    void power(const F32 val);

   private:
     void onToggle(const bool state) final;

   private:
    PushConstants _ssaoBlurConstants;
    PushConstants _ssaoGenerateConstants;
    ShaderProgram_ptr _ssaoGenerateShader = nullptr;
    ShaderProgram_ptr _ssaoBlurShader = nullptr;
    Texture_ptr _noiseTexture = nullptr;
    RenderTargetHandle _ssaoOutput;
    F32 _radius = 1.0f;
    F32 _power = 1.0f;
    bool _enabled = true;
};

};  // namespace Divide

#endif