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

#ifndef _RENDER_API_H_
#define _RENDER_API_H_

#include "RenderDrawCommands.h"

namespace std {
    class thread::id;
};

namespace Divide {

enum class ErrorCode : I32;

template <typename T>
class vec4;

class Configuration;
struct GUITextBatchEntry;

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

typedef std::array<bool, to_base(VertexAttribute::COUNT)> AttribFlags;

/// Renderer Programming Interface
class NOINITVTABLE RenderAPIWrapper : private NonCopyable {
   protected:
    friend class GFXDevice;
    /*Application display frame*/
    /// Clear buffers, set default states, etc
    virtual void beginFrame() = 0;
    /// Clear shaders, restore active texture units, etc
    virtual void endFrame(bool swapBuffers) = 0;

    virtual ErrorCode initRenderingAPI(I32 argc, char** argv, const Configuration& config) = 0;
    virtual void closeRenderingAPI() = 0;

    virtual void drawText(const vectorImpl<GUITextBatchEntry>& batch) = 0;

    // a debug message is a marker that should show up in external profiling tools such as RenderDoc or PerfStudio /NSight
    virtual void pushDebugMessage(const char* message, I32 id) = 0;
    virtual void popDebugMessage() = 0;

    virtual void updateClipPlanes() = 0;
    virtual U64  getFrameDurationGPU() = 0;

    virtual size_t setStateBlock(size_t stateBlockHash) = 0;
    virtual bool draw(const GenericDrawCommand& cmd) = 0;

    virtual void flushCommandBuffer(const CommandBuffer& commandBuffer) = 0;

   protected:
    virtual void changeViewport(const vec4<I32>& newViewport) const = 0;
    virtual void syncToThread(const std::thread::id& threadID) = 0;
    virtual void registerCommandBuffer(const ShaderBuffer& commandBuffer) const = 0;
};

};  // namespace Divide

#endif
