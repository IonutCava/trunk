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

#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

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

class ShaderProgram;
class GenericVertexData;
class VertexDataInterface;

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

struct IndirectDrawCommand {
    IndirectDrawCommand()
        : indexCount(0),
          primCount(1),
          firstIndex(0),
          baseVertex(0),
          baseInstance(0)
    {
    }

    U32 indexCount;
    U32 primCount;
    U32 firstIndex;
    U32 baseVertex;
    U32 baseInstance;

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
        RENDER_BOUNDS = toBit(3)
    };

   private:
    U8 _lodIndex;
    U16 _drawCount;
    U32 _renderOptions;
    U32 _drawToBuffer;
    U32 _stateHash;
    U32 _commandOffset;
    PrimitiveType _type;
    IndirectDrawCommand _cmd;
    ShaderProgram* _shaderProgram;
    VertexDataInterface* _sourceBuffer;

   public:

    inline void drawCount(U16 count) { 
        _drawCount = count; 
    }

    inline void LoD(U8 lod) {
        _lodIndex = lod;
    }

    inline void stateHash(U32 hashValue) {
        _stateHash = hashValue;
    }

    inline void drawToBuffer(U32 index) {
        _drawToBuffer = index;
    }

    inline void renderWireframe(bool state) {
        state ? SetBit(_renderOptions, to_const_uint(RenderOptions::RENDER_WIREFRAME))
              : ClearBit(_renderOptions, to_const_uint(RenderOptions::RENDER_WIREFRAME));
    }

    inline void renderBounds(bool state) {
        state ? SetBit(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS))
              : ClearBit(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS));
    }

    inline void renderGeometry(bool state) {
        state ? SetBit(_renderOptions, to_const_uint(RenderOptions::RENDER_GEOMETRY))
              : ClearBit(_renderOptions, to_const_uint(RenderOptions::RENDER_GEOMETRY));
    }

    inline void shaderProgram(ShaderProgram* const program) {
        _shaderProgram = program;
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
    inline U32 stateHash() const { return _stateHash; }
    inline U32 drawToBuffer() const { return _drawToBuffer; }
    inline U32 commandOffset() const { return _commandOffset; }

    inline bool renderWireframe() const {
        return BitCompare(_renderOptions, to_const_uint(RenderOptions::RENDER_WIREFRAME));
    }
    inline bool renderGeometry() const {
        return BitCompare(_renderOptions, to_const_uint(RenderOptions::RENDER_GEOMETRY));
    }
    inline bool renderBounds() const {
        return BitCompare(_renderOptions, to_const_uint(RenderOptions::RENDER_BOUNDS));
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

    GenericDrawCommand(const PrimitiveType& type, U32 firstIndex, U32 indexCount,
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
    inline U32 getHash() const {
        U32 hash = 0;
        Util::Hash_combine(hash, to_uint(_textureType));
        Util::Hash_combine(hash, _samplerHash);
        Util::Hash_combine(hash, _textureHandle);
        return hash;
    }

    TextureType _textureType;
    U32 _samplerHash;
private:
    U64  _textureHandle;
};

inline GFXImageFormat baseFromInternalFormat(GFXImageFormat internalFormat) {
    switch (internalFormat) {
        case GFXImageFormat::LUMINANCE_ALPHA:
        case GFXImageFormat::LUMINANCE_ALPHA16F:
        case GFXImageFormat::LUMINANCE_ALPHA32F:
            return GFXImageFormat::LUMINANCE;

        case GFXImageFormat::RED8:
        case GFXImageFormat::RED16:
        case GFXImageFormat::RED16F:
        case GFXImageFormat::RED32:
        case GFXImageFormat::RED32F:
            return GFXImageFormat::RED;

        case GFXImageFormat::RG8:
        case GFXImageFormat::RG16:
        case GFXImageFormat::RG16F:
        case GFXImageFormat::RG32:
        case GFXImageFormat::RG32F:
            return GFXImageFormat::RG;

        case GFXImageFormat::RGB8:
        case GFXImageFormat::SRGB8:
        case GFXImageFormat::RGB8I:
        case GFXImageFormat::RGB16:
        case GFXImageFormat::RGB16F:
        case GFXImageFormat::RGB32F:
            return GFXImageFormat::RGB;

        case GFXImageFormat::RGBA4:
        case GFXImageFormat::RGBA8:
        case GFXImageFormat::SRGBA8:
        case GFXImageFormat::RGBA8I:
        case GFXImageFormat::RGBA16F:
        case GFXImageFormat::RGBA32F:
            return GFXImageFormat::RGBA;

        case GFXImageFormat::DEPTH_COMPONENT16:
        case GFXImageFormat::DEPTH_COMPONENT24:
        case GFXImageFormat::DEPTH_COMPONENT32:
        case GFXImageFormat::DEPTH_COMPONENT32F:
            return GFXImageFormat::DEPTH_COMPONENT;

        default:
            break;
    };

    return internalFormat;
}

inline GFXDataFormat dataTypeForInternalFormat(GFXImageFormat format) {
    switch (format) {
        case GFXImageFormat::DEPTH_COMPONENT32F:
        case GFXImageFormat::LUMINANCE_ALPHA32F:
        case GFXImageFormat::RED32F:
        case GFXImageFormat::RG32F:
        case GFXImageFormat::RGB32F:
        case GFXImageFormat::RGBA32F:
            return GFXDataFormat::FLOAT_32;

        case GFXImageFormat::LUMINANCE_ALPHA16F:
        case GFXImageFormat::RED16F:
        case GFXImageFormat::RG16F:
        case GFXImageFormat::RGB16F:
        case GFXImageFormat::RGBA16F:
            return GFXDataFormat::FLOAT_16;

        case GFXImageFormat::RGB8I:
        case GFXImageFormat::RGBA8I:
            return GFXDataFormat::SIGNED_BYTE;
    };

    return GFXDataFormat::UNSIGNED_BYTE;
}

typedef vectorImpl<TextureData> TextureDataContainer;
typedef std::array<IndirectDrawCommand, Config::MAX_VISIBLE_NODES> DrawCommandList;

enum class ShaderType : U32;
class Shader;
class Texture;
class TextureData;
class IMPrimitive;
class PixelBuffer;
class Framebuffer;
class ShaderBuffer;
class VertexBuffer;
class ShaderProgram;

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

        const inline U32 queueLength() const { return _queueLength; }
        const inline U32 queueWriteIndex() const { return _queueWriteIndex; }
        const inline U32 queueReadIndex() const { return _queueReadIndex; }

        inline void incQueue() { 
            if (queueLength() > 1) {
                _queueWriteIndex = ++_queueWriteIndex % _queueLength;
                _queueReadIndex = ++_queueReadIndex % _queueLength;
            }
        }

        inline void decQueue() {
            if (queueLength() > 1) {
                _queueWriteIndex = --_queueWriteIndex % _queueLength;
                _queueReadIndex = --_queueReadIndex % _queueLength;
            }
        }

    private:
        const U32 _queueLength;
        U32 _queueReadIndex;
        U32 _queueWriteIndex;

};

/// Renderer Programming Interface
class NOINITVTABLE RenderAPIWrapper : private NonCopyable {
   protected:
    friend class GFXDevice;
    /*Application display frame*/
    /// Clear buffers, set default states, etc
    virtual void beginFrame() = 0;
    /// Clear shaders, restore active texture units, etc
    virtual void endFrame() = 0;
    /// Platform specific cursor manipulation.
    /// Set's the cursor's location to the specified X and Y relative to the
    /// edge of the window
    virtual void setCursorPosition(I32 x, I32 y) = 0;
    virtual IMPrimitive* newIMP(GFXDevice& context) const = 0;
    virtual Framebuffer* newFB(GFXDevice& context, bool multisampled) const = 0;
    virtual VertexBuffer* newVB(GFXDevice& context) const = 0;
    virtual ShaderBuffer* newSB(GFXDevice& context,
                                const stringImpl& bufferName,
                                const U32 ringBufferLength = 1,
                                const bool unbound = false,
                                const bool persistentMapped = true,
                                BufferUpdateFrequency frequency =
                                    BufferUpdateFrequency::ONCE) const = 0;
    virtual GenericVertexData* newGVD(GFXDevice& context, 
                                      const bool persistentMapped = false,
                                      const U32 ringBufferLength = 1) const = 0;
    virtual PixelBuffer* newPB(GFXDevice& context,
                               const PBType& type = PBType::PB_TEXTURE_2D) const = 0;
    virtual Texture* newTexture(GFXDevice& context, TextureType type, bool asyncLoad) const = 0;
    virtual ShaderProgram* newShaderProgram(GFXDevice& context, bool asyncLoad) const = 0;
    virtual Shader* newShader(GFXDevice& context,
                              const stringImpl& name, const ShaderType& type,
                              const bool optimise = false) const = 0;

    virtual bool initShaders() = 0;
    virtual bool deInitShaders() = 0;

    virtual ErrorCode initRenderingAPI(I32 argc, char** argv) = 0;
    virtual void closeRenderingAPI() = 0;


    virtual void toggleDepthWrites(bool state) = 0;
    virtual void toggleRasterization(bool state) = 0;
    virtual void drawText(const TextLabel& textLabel,
                          const vec2<F32>& relativeOffset) = 0;

    virtual void updateClipPlanes() = 0;
    virtual U64  getFrameDurationGPU() = 0;
    virtual void activateStateBlock(const RenderStateBlock& newBlock,
                                    const RenderStateBlock& oldBlock) const = 0;

    virtual void drawPoints(U32 numPoints) = 0;
    virtual void drawTriangle() = 0;

   protected:
    virtual void changeResolution(U16 w, U16 h) = 0;
    virtual void changeViewport(const vec4<I32>& newViewport) const = 0;
    virtual void threadedLoadCallback() = 0;
    virtual void registerCommandBuffer(const ShaderBuffer& commandBuffer) const = 0;

    virtual bool makeTexturesResident(const TextureDataContainer& textureData) = 0;
    virtual bool makeTextureResident(const TextureData& textureData) = 0;
};

};  // namespace Divide

#endif
