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
#ifndef _VK_WRAPPER_H_
#define _VK_WRAPPER_H_

#include "config.h"
#include "vkPlaceholderObjects.h"

#include "Platform/Video/Headers/RenderAPIWrapper.h"
namespace Divide {

class VK_API final : public RenderAPIWrapper {
  public:
    VK_API(GFXDevice& context);
    ~VK_API();

  protected:
      void idle() final;
      void beginFrame(DisplayWindow& window, bool global = false) final;
      void endFrame(DisplayWindow& window, bool global = false) final;
      RenderAPI renderAPI() const final;
      ErrorCode initRenderingAPI(I32 argc, char** argv, Configuration& config) final;
      void closeRenderingAPI() final;
      F32 getFrameDurationGPU() const final;
      size_t setStateBlock(size_t stateBlockHash) final;
      void flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) final;
      void preFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) final;
      void postFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer, bool submitToGPU) final;
      vec2<U16> getDrawableSize(const DisplayWindow& window) const final;
      U32 getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const final;
      bool setViewport(const Rect<I32>& newViewport) final;
      void onThreadCreated(const std::thread::id& threadID) final;
};

};  // namespace Divide
#endif //_VK_WRAPPER_H_
