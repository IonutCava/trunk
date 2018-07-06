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

#ifndef _RENDER_API_H_
#define _RENDER_API_H_

#include "RenderDrawCommands.h"

namespace std {
    class thread::id;
};

namespace Divide {

class Kernel;
class Light;
class SubMesh;

class Material;
class Object3D;
class TextLabel;
class Transform;
class GUIElement;
class RenderStateBlock;
class SceneRenderState;

struct GUITextBatchEntry;

enum class ErrorCode : I32;

template <typename T>
class Plane;
typedef vectorImpl<Plane<F32> > PlaneList;

template <typename T>
class vec4;

class SceneGraph;

class GenericVertexData;
class VertexDataInterface;

FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

typedef struct {
    // Video resolution
    I32 Width, Height;
    // Red bits per pixel
    I32 RedBits;
    // Green bits per pixel
    I32 GreenBits;
    // Blue bits per pixel
    I32 BlueBits;
} VideoModes;
// FWD DECLARE CLASSES


class RingBuffer {
    public:
        explicit RingBuffer(U32 queueLength) : 
            _queueLength(std::max(queueLength, 1U))
        {
            _queueReadIndex = 0;
            _queueWriteIndex = _queueLength - 1;
        }

        RingBuffer(const RingBuffer& other)
            : _queueLength(other._queueLength)
        {
            _queueReadIndex = other.queueReadIndex();
            _queueWriteIndex = other.queueWriteIndex();
        }

        virtual ~RingBuffer()
        {
        }

        const inline U32 queueLength() const {
            return _queueLength;
        }

        const inline U32 queueWriteIndex() const {
            return _queueWriteIndex;
        }

        const inline U32 queueReadIndex() const {
            return _queueReadIndex;
        }

        virtual void incQueue() { 
            if (queueLength() > 1) {
                _queueWriteIndex = (_queueWriteIndex + 1) % _queueLength;
                _queueReadIndex  = (_queueReadIndex + 1) % _queueLength;
            }
        }

        inline void decQueue() {
            if (queueLength() > 1) {
                _queueWriteIndex = (_queueWriteIndex - 1) % _queueLength;
                _queueReadIndex = (_queueReadIndex - 1) % _queueLength;
            }
        }

    private:
        const U32 _queueLength;
        std::atomic_uint _queueReadIndex;
        std::atomic_uint _queueWriteIndex;

};

/// Renderer Programming Interface
class NOINITVTABLE RenderAPIWrapper : private NonCopyable {
   protected:
    friend class GFXDevice;
    /*Application display frame*/
    /// Clear buffers, set default states, etc
    virtual void beginFrame() = 0;
    /// Clear shaders, restore active texture units, etc
    virtual void endFrame(bool swapBuffers) = 0;

    virtual ErrorCode initRenderingAPI(I32 argc, char** argv) = 0;
    virtual void closeRenderingAPI() = 0;

    virtual void drawText(const vectorImpl<GUITextBatchEntry>& batch) = 0;

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
