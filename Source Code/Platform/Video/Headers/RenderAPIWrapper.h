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
#ifndef _RENDER_API_H_
#define _RENDER_API_H_

#include "CommandBuffer.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace std {
    class thread::id;
};

namespace CEGUI {
    class Texture;
};

namespace Divide {

enum class ErrorCode : I8;

template <typename T>
class vec4;

class DisplayWindow;

struct Configuration;
struct TextElementBatch;

FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

struct VideoModes {
    // Video resolution
    I32 Width, Height;
    // Red bits per pixel
    I32 RedBits;
    // Green bits per pixel
    I32 GreenBits;
    // Blue bits per pixel
    I32 BlueBits;
};

using AttribFlags = std::array<bool, to_base(AttribLocation::COUNT)>;

/// Renderer Programming Interface
class NOINITVTABLE RenderAPIWrapper : private NonCopyable {

public:
    virtual ~RenderAPIWrapper() = default;

protected:
    friend class GFXDevice;
    virtual void idle() = 0;

    /*Application display frame*/
    /// Clear buffers, set default states, etc
    virtual void beginFrame(DisplayWindow& window, bool global = false) = 0;
    /// Clear shaders, restore active texture units, etc
    virtual void endFrame(DisplayWindow& window, bool global = false) = 0;

    virtual RenderAPI renderAPI() const = 0;
    virtual ErrorCode initRenderingAPI(I32 argc, char** argv, Configuration& config) = 0;
    virtual void closeRenderingAPI() = 0;

    // Returns the time in milliseconds that it took to render one frame
    // Queries are expensive, so this result MAY BE SEVERAL frames out of date!
    virtual F32 getFrameDurationGPU() const = 0;

    virtual size_t setStateBlock(size_t stateBlockHash) = 0;

    virtual void flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) = 0;

    virtual void preFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) = 0;
    virtual void postFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) = 0;

    virtual vec2<U16> getDrawableSize(const DisplayWindow& window) const = 0;

    // The definition of a hack. Feel free to quote this. -Ionut
    /// Convert a CEGUI texture handle to something that our current rendering API can use
    virtual U32 getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const = 0;

    virtual bool setViewport(const Rect<I32>& newViewport) = 0;

    virtual void onThreadCreated(const std::thread::id& threadID) = 0;
};

};  // namespace Divide

#endif
