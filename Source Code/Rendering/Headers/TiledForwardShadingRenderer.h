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

#ifndef _FORWARD_PLUS_RENDERER_H_
#define _FORWARD_PLUS_RENDERER_H_

#include "Renderer.h"

namespace Divide {

class TiledForwardShadingRenderer : public Renderer {
   public:
    TiledForwardShadingRenderer();
    ~TiledForwardShadingRenderer();

    void preRender(RenderTarget& target, LightPool& lightPool) override;

    void render(const DELEGATE_CBK<>& renderCallback,
                const SceneRenderState& sceneRenderState)  override;

    void updateResolution(U16 width, U16 height)  override;

   private:
       U32 getMaxNumLightsPerTile() const;
       U32 getNumTilesX() const;
       U32 getNumTilesY() const;

   private:
    vec2<U16> _resolution;
    ShaderProgram_ptr _lightCullComputeShader;
    std::shared_ptr<ShaderBuffer> _perTileLightIndexBuffer;
};

};  // namespace Divide

#endif