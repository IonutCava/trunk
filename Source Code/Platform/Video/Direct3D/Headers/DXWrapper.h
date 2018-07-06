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

#ifndef _WRAPPER_DX_H_
#define _WRAPPER_DX_H_

#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Direct3D/Headers/d3dEnumTable.h"

namespace Divide {

class DX_API final : public RenderAPIWrapper {
public:
    DX_API(GFXDevice& context) : RenderAPIWrapper()
    {
        ACKNOWLEDGE_UNUSED(context);
    }

    virtual ~DX_API()
    {
    }

protected:
    ErrorCode initRenderingAPI(I32 argc, char** argv, const Configuration& config) override;
    void closeRenderingAPI() override;
    void changeViewport(const vec4<I32>& newViewport) const override;
    void registerCommandBuffer(const ShaderBuffer& commandBuffer) const override;
    void beginFrame() override;
    void endFrame(bool swapBuffers) override;

    void drawText(const vectorImpl<GUITextBatchEntry>& batch) override;
    bool draw(const GenericDrawCommand& cmd);

    void flushCommandBuffer(const CommandBuffer& commandBuffer) override;

    void updateClipPlanes() override;

    size_t setStateBlock(size_t stateBlockHash) override;

    U64 getFrameDurationGPU() override { return 0; }

    void pushDebugMessage(const char* message, I32 id) override;

    void popDebugMessage() override;
};

};  // namespace Divide
#endif
