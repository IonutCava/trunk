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

#include "config.h"

#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

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

struct IndirectDrawCommand {
    IndirectDrawCommand()
        : indexCount(0),
          primCount(1),
          firstIndex(0),
          baseVertex(0),
          baseInstance(0)
    {
    }

    U32 indexCount;    // 4  bytes
    U32 primCount;     // 8  bytes
    U32 firstIndex;    // 12 bytes
    U32 baseVertex;    // 20 bytes
    U32 baseInstance;  // 24 bytes

    inline void set(const IndirectDrawCommand& other) {
        indexCount = other.indexCount;
        primCount = other.primCount;
        firstIndex = other.firstIndex;
        baseVertex = other.baseVertex;
        baseInstance = other.baseInstance;
    }

    bool operator==(const IndirectDrawCommand &other) const {
        return  indexCount == other.indexCount &&
                primCount == other.primCount &&
                firstIndex == other.firstIndex &&
                baseVertex == other.baseVertex &&
                baseInstance == other.baseInstance;
    }
};

struct GenericDrawCommand {
   public:
    enum class RenderOptions : U32 {
        RENDER_GEOMETRY = toBit(1),
        RENDER_WIREFRAME = toBit(2),
        RENDER_BOUNDS_AABB = toBit(3),
        RENDER_BOUNDS_SPHERE = toBit(4),
        COUNT = 4
    };

   private:
    // state hash is not size_t to avoid any platform specific awkward typedefing
    IndirectDrawCommand _cmd;           // 64 bytes
    U64 _stateHash;                     // 44 bytes
    ShaderProgram* _shaderProgram;      // 36 bytes
    VertexDataInterface* _sourceBuffer; // 28 bytes
    PrimitiveType _type;                // 20 bytes
    U32 _commandOffset;                 // 16 bytes
    U32 _renderOptions;                 // 12 bytes
    U16 _drawCount;                     // 4  bytes
    U8 _drawToBuffer;                   // 2  bytes
    U8 _lodIndex;                       // 1  bytes

   public:

    inline void drawCount(U16 count) { 
        _drawCount = count; 
    }

    inline void LoD(U8 lod) {
        _lodIndex = lod;
    }

    inline void stateHash(size_t hashValue) {
        _stateHash = static_cast<U64>(hashValue);
    }

    inline void drawToBuffer(U8 index) {
        _drawToBuffer = index;
    }

    inline void renderMask(U32 mask) {
        _renderOptions = mask;
    }
    
    inline void renderWireframe(bool state) {
        state ? SetBit(_renderOptions, to_const_uint(RenderOptions::RENDER_WIREFRAME))
              : ClearBit(_renderOptions, to_const_uint(RenderOptions::RENDER_WIREFRAME));
    }

    inline void renderGeometry(bool state) {
        state ? SetBit(_renderOptions, to_const_uint(RenderOptions::RENDER_GEOMETRY))
              : ClearBit(_renderOptions, to_const_uint(RenderOptions::RENDER_GEOMETRY));
    }

    inline void renderBoundsAABB(bool state) {
        state ? SetBit(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS_AABB))
              : ClearBit(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS_AABB));
    }

    inline void renderBoundsSphere(bool state) {
        state ? SetBit(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS_SPHERE))
              : ClearBit(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS_SPHERE));
    }

    inline void shaderProgram(const ShaderProgram_ptr& program) {
        _shaderProgram = program.get();
    }

    inline void sourceBuffer(VertexDataInterface* const sourceBuffer) {
        _sourceBuffer = sourceBuffer;
    }

    inline void primitiveType(PrimitiveType type) {
        _type = type;
    }

    inline void commandOffset(U32 offset) {
        _commandOffset = offset;
    }

    inline U8 LoD() const { return _lodIndex; }
    inline U16 drawCount() const { return _drawCount; }
    inline size_t stateHash() const { return static_cast<size_t>(_stateHash); }
    inline U8  drawToBuffer() const { return _drawToBuffer; }
    inline U32 commandOffset() const { return _commandOffset; }

    inline bool renderWireframe() const {
        return BitCompare(_renderOptions, to_const_uint(RenderOptions::RENDER_WIREFRAME));
    }

    inline bool renderGeometry() const {
        return BitCompare(_renderOptions, to_const_uint(RenderOptions::RENDER_GEOMETRY));
    }

    inline bool renderBoundsAABB() const {
        return BitCompare(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS_AABB));
    }

    inline bool renderBoundsSphere() const {
        return BitCompare(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS_SPHERE));
    }

    inline const IndirectDrawCommand& cmd() const {
        return _cmd;
    }

    inline IndirectDrawCommand& cmd() {
        return _cmd;
    }

    inline ShaderProgram* shaderProgram() const { return _shaderProgram; }
    inline VertexDataInterface* sourceBuffer() const { return _sourceBuffer; }
    inline PrimitiveType primitiveType() const { return _type; }

    GenericDrawCommand()
        : GenericDrawCommand(PrimitiveType::TRIANGLE_STRIP, 0, 0)
    {
    }

    GenericDrawCommand(PrimitiveType type,
                       U32 firstIndex,
                       U32 indexCount,
                       U32 primCount = 1)
        : _lodIndex(0),
          _drawCount(1),
          _drawToBuffer(0),
          _stateHash(0),
    	  _type(type),
          _commandOffset(0),
          _shaderProgram(nullptr),
          _sourceBuffer(nullptr)
    {
        SetBit(_renderOptions, to_const_uint(RenderOptions::RENDER_GEOMETRY));
        _cmd.indexCount = indexCount;
        _cmd.firstIndex = firstIndex;
        _cmd.primCount = primCount;

        static_assert(sizeof(IndirectDrawCommand) == 20, "Size of IndirectDrawCommand is incorrect!");
        static_assert(sizeof(GenericDrawCommand) == 64, "Size of GenericDrawCommand is incorrect!");
    }

    inline void set(const GenericDrawCommand& base) {
        _cmd.set(base._cmd);
        _lodIndex = base._lodIndex;
        _drawCount = base._drawCount;
        _drawToBuffer = base._drawToBuffer;
        _renderOptions = base._renderOptions;
        _stateHash = base._stateHash;
        _type = base._type;
        _shaderProgram = base._shaderProgram;
        _sourceBuffer = base._sourceBuffer;
        _commandOffset = base._commandOffset;
    }

    inline void reset() {
        set(GenericDrawCommand());
    }

    inline bool compatible(const GenericDrawCommand& other) const {
        return _lodIndex == other._lodIndex &&
               _drawToBuffer == other._drawToBuffer &&
               _renderOptions == other._renderOptions &&
               _stateHash == other._stateHash && _type == other._type &&
               (_shaderProgram != nullptr) == (other._shaderProgram != nullptr) &&
               (_sourceBuffer != nullptr) == (other._sourceBuffer != nullptr);
    }
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

    virtual void toggleDepthWrites(bool state) = 0;
    virtual void toggleRasterization(bool state) = 0;
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
