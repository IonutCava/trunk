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

#ifndef _WRAPPER_DX_H_
#define _WRAPPER_DX_H_

#include "Platform/Video/Headers/RenderAPIWrapper.h"

#include "Platform/Video/Direct3D/Headers/d3dEnumTable.h"

namespace Divide {

DEFINE_SINGLETON_W_SPECIFIER(DX_API, RenderAPIWrapper, final)
  protected:
    DX_API() : RenderAPIWrapper() {}
    virtual ~DX_API() {}

    ErrorCode initRenderingAPI(I32 argc, char** argv) override;
    void closeRenderingAPI() override;
    void changeViewport(const vec4<I32>& newViewport) const override;
    void registerCommandBuffer(const ShaderBuffer& commandBuffer) const override;
    bool makeTexturesResident(const TextureDataContainer& textureData) override;
    bool makeTextureResident(const TextureData& textureData) override;
    void beginFrame() override;
    void endFrame(bool swapBuffers) override;

    void toggleDepthWrites(bool state) override;
    void toggleRasterization(bool state) override;

    void drawText(const TextLabel& textLabel, const vec2<F32>& position) override;
    void draw(const GenericDrawCommand& cmd);
    void updateClipPlanes() override;
    void syncToThread(std::thread::id threadID) override;

    U64 getFrameDurationGPU() override { return 0; }
    void activateStateBlock(const RenderStateBlock& newBlock,
                            const RenderStateBlock& oldBlock) const override;

END_SINGLETON

};  // namespace Divide
#endif
