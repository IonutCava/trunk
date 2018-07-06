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

#ifndef _DEF_SHADE_RENDERER_H_
#define _DEF_SHADE_RENDERER_H_

#include "Renderer.h"

namespace Divide {

class Quad3D;
class RenderTarget;
class PixelBuffer;

/// This class implements a full  deferred renderer based on the 2 pass, huge
/// G-buffer model
class DeferredShadingRenderer : public Renderer {
   public:
    DeferredShadingRenderer(PlatformContext& context, ResourceCache& cache);
    ~DeferredShadingRenderer();

    void preRender(RenderTarget& target,
                   LightPool& lightPool,
                   GFX::CommandBuffer& bufferInOut) override;

    void render(const DELEGATE_CBK<void, GFX::CommandBuffer&>& renderCallback,
                const SceneRenderState& sceneRenderState,
                GFX::CommandBuffer& bufferInOut) override;

    void updateResolution(U16 width, U16 height) override;

   private:
    void firstPass(const DELEGATE_CBK<void, GFX::CommandBuffer&>& renderCallback,
                   const SceneRenderState& sceneRenderState,
                   GFX::CommandBuffer& bufferInOut);
    void secondPass(const SceneRenderState& sceneRenderState,
                    GFX::CommandBuffer& bufferInOut);

   private:
    U16 _cachedLightCount;
    vectorImpl<std::shared_ptr<Quad3D>> _renderQuads;
    RenderTargetHandle _deferredBuffer;
    ShaderProgram_ptr _deferredShader;
    ShaderProgram_ptr _previewDeferredShader;
    PixelBuffer* _lightTexture;
};

};  // namespace Divide

#endif