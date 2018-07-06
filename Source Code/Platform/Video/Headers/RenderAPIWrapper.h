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

#include "RenderAPIEnums.h"
#include "GenericDrawCommand.h"
#include "Core/Math/Headers/MathMatrices.h"

#include <thread>

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
enum class ErrorCode : I32;

template <typename T>
class Plane;
typedef vectorImpl<Plane<F32> > PlaneList;
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

enum class RenderAPI : U32 {
    OpenGL,    ///< 4.x+
    OpenGLES,  ///< 3.x+
    Direct3D,  ///< 12.x+ (not supported yet)
    Vulkan,    ///< not supported yet
    None,      ///< not supported yet
    COUNT
};


class TextureData {
    public:
    TextureData()
        : _textureType(TextureType::TEXTURE_2D),
          _samplerHash(0),
          _textureHandle(0)
    {
    }

    TextureData(const TextureData& other)
        : _textureType(other._textureType),
          _samplerHash(other._samplerHash),
          _textureHandle(other._textureHandle)
    {
    }

    TextureData& operator=(const TextureData& other) {
        _textureType = other._textureType;
        _samplerHash = other._samplerHash;
        _textureHandle = other._textureHandle;

        return *this;
    }

    inline void set(const TextureData& other) {
        _textureHandle = other._textureHandle;
        _textureType = other._textureType;
        _samplerHash = other._samplerHash;
    }

    inline void setHandleHigh(U32 handle) {
        _textureHandle = (U64)handle << 32 | getHandleLow();
    }

    inline U32 getHandleHigh() const {
        return to_uint(_textureHandle >> 32);
    }

    inline void getHandleHigh(U32& handle) const {
        handle = getHandleHigh();
    }

    inline void setHandleLow(U32 handle) {
        _textureHandle |= handle;
    }

    inline U32 getHandleLow() const{
        return to_uint(_textureHandle);
    }
        
    inline void getHandleLow(U32& handle) const {
        handle = getHandleLow();
    }

    inline void setHandle(U64 handle) {
        _textureHandle = handle;
    }
        
    inline void getHandle(U64& handle) const {
        handle = _textureHandle;
    }

    // No need to cache this as it should already be pretty fast
    inline size_t getHash() const {
        size_t hash = 0;
        Util::Hash_combine(hash, to_uint(_textureType));
        Util::Hash_combine(hash, _samplerHash);
        Util::Hash_combine(hash, _textureHandle);
        return hash;
    }

    TextureType _textureType;
    size_t _samplerHash;
private:
    U64  _textureHandle;
};

typedef vectorImpl<TextureData> TextureDataContainer;
typedef std::array<IndirectDrawCommand, Config::MAX_VISIBLE_NODES> DrawCommandList;


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
            _queueReadIndex = other._queueReadIndex;
            _queueWriteIndex = other._queueWriteIndex;
        }

        virtual ~RingBuffer()
        {
        }

        const inline U32 queueLength() const {
            ReadLock r_lock(_lock);
            return _queueLength;
        }

        const inline U32 queueWriteIndex() const {
            ReadLock r_lock(_lock);
            return _queueWriteIndex;
        }

        const inline U32 queueReadIndex() const {
            ReadLock r_lock(_lock);
            return _queueReadIndex;
        }

        virtual void incQueue() { 
            if (queueLength() > 1) {
                WriteLock w_lock(_lock);
                _queueWriteIndex = (_queueWriteIndex + 1) % _queueLength;
                _queueReadIndex  = (_queueReadIndex + 1) % _queueLength;
            }
        }

        inline void decQueue() {
            if (queueLength() > 1) {
                WriteLock w_lock(_lock);
                _queueWriteIndex = (_queueWriteIndex - 1) % _queueLength;
                _queueReadIndex = (_queueReadIndex - 1) % _queueLength;
            }
        }

    private:
        const U32 _queueLength;
        U32 _queueReadIndex;
        U32 _queueWriteIndex;
        mutable SharedLock _lock;

};

class ShaderBuffer;
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

    virtual void drawText(const TextLabel& textLabel,
                          const vec2<F32>& position) = 0;

    virtual void updateClipPlanes() = 0;
    virtual U64  getFrameDurationGPU() = 0;
    virtual void activateStateBlock(const RenderStateBlock& newBlock,
                                    const RenderStateBlock& oldBlock) const = 0;

    virtual void draw(const GenericDrawCommand& cmd) = 0;

   protected:
    virtual void changeViewport(const vec4<I32>& newViewport) const = 0;
    virtual void syncToThread(std::thread::id threadID) = 0;
    virtual void registerCommandBuffer(const ShaderBuffer& commandBuffer) const = 0;

    virtual bool makeTexturesResident(const TextureDataContainer& textureData) = 0;
    virtual bool makeTextureResident(const TextureData& textureData) = 0;
};

};  // namespace Divide

#endif
