/*
   Copyright (c) 2015 DIVIDE-Studio
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
        : count(0),
          primCount(1),
          firstIndex(0),
          baseVertex(0),
          baseInstance(0)
    {
    }

    U32 count;
    U32 primCount;
    U32 firstIndex;
    U32 baseVertex;
    U32 baseInstance;

    inline void set(const IndirectDrawCommand& other) {
        count = other.count;
        primCount = other.primCount;
        firstIndex = other.firstIndex;
        baseVertex = other.baseVertex;
        baseInstance = other.baseInstance;
    }
};

struct GenericDrawCommand {
   private:
    U8 _queryID;
    U8 _lodIndex;
    U16 _drawCount;
    bool _locked;
    bool _drawToBuffer;
    bool _renderWireframe;
    bool _renderGeometry;
    size_t _stateHash;
    PrimitiveType _type;
    IndirectDrawCommand _cmd;
    ShaderProgram* _shaderProgram;
    VertexDataInterface* _sourceBuffer;

   public:
    inline void lock() { _locked = true; }

    inline void drawCount(U16 count) { 
        assert(!_locked);
        _drawCount = count; 
    }

    inline void drawID(U32 ID) {
        assert(!_locked);
        _cmd.baseInstance = ID;
    }

    inline void LoD(U8 lod) {
        assert(!_locked);
        _lodIndex = lod;
    }

    inline void queryID(U8 queryID) {
        assert(!_locked);
        _queryID = queryID;
    }

    inline void stateHash(size_t hashValue) {
        assert(!_locked);
        _stateHash = hashValue;
    }

    inline void drawToBuffer(bool state) {
        assert(!_locked);
        _drawToBuffer = state;
    }

    inline void renderWireframe(bool state) {
        assert(!_locked);
        _renderWireframe = state;
    }

    inline void renderGeometry(bool state) {
        assert(!_locked);
        _renderGeometry = state;
    }

    inline void primCount(U32 count) {
        assert(!_locked);
        _cmd.primCount = count;
    }

    inline void firstIndex(U32 index) {
        assert(!_locked);
        _cmd.firstIndex = index;
    }

    inline void indexCount(U32 count) {
        assert(!_locked);
        _cmd.count = count;
    }

    inline void shaderProgram(ShaderProgram* const program) {
        assert(!_locked);
        _shaderProgram = program;
    }

    inline void sourceBuffer(VertexDataInterface* const sourceBuffer) {
        assert(!_locked);
        _sourceBuffer = sourceBuffer;
    }

    inline void primitiveType(PrimitiveType type) {
        assert(!_locked);
        _type = type;
    }

    inline U8 LoD() const { return _lodIndex; }
    inline U8 queryID() const { return _queryID; }
    inline U32 drawID() const { return _cmd.baseInstance; }
    inline U16 drawCount() const { return _drawCount; }
    inline size_t stateHash() const { return _stateHash; }
    inline bool drawToBuffer() const { return _drawToBuffer; }
    inline bool renderWireframe() const { return _renderWireframe; }
    inline bool renderGeometry() const { return _renderGeometry; }
    inline U32 primCount() const { return _cmd.primCount; }
    inline U32 indexCount() const { return _cmd.count; }

    inline const IndirectDrawCommand& cmd() const { return _cmd; }
    inline ShaderProgram* shaderProgram() const { return _shaderProgram; }
    inline VertexDataInterface* sourceBuffer() const { return _sourceBuffer; }
    inline PrimitiveType primitiveType() const { return _type; }

    GenericDrawCommand()
        : GenericDrawCommand(PrimitiveType::TRIANGLE_STRIP, 0, 0)
    {
    }

    GenericDrawCommand(const PrimitiveType& type, U32 firstIndex, U32 count,
                       U32 primCount = 1)
        : _queryID(0),
          _lodIndex(0),
          _drawCount(1),
          _locked(false),
          _drawToBuffer(false),
          _renderWireframe(false),
          _renderGeometry(true),
          _stateHash(0),
    	  _type(type),
          _shaderProgram(nullptr),
          _sourceBuffer(nullptr)
    {
        _cmd.count = count;
        _cmd.firstIndex = firstIndex;
        _cmd.primCount = primCount;
    }

    inline void set(const GenericDrawCommand& base)
    {
        assert(!_locked);
        _cmd.set(base._cmd);

        _queryID = base._queryID;
        _lodIndex = base._lodIndex;
        _drawCount = base._drawCount;
        _drawToBuffer = base._drawToBuffer;
        _renderWireframe = base._renderWireframe;
        _renderGeometry = base._renderGeometry;
        _stateHash = base._stateHash;
        _type = base._type;
        _shaderProgram = base._shaderProgram;
        _sourceBuffer = base._sourceBuffer;
    }

    inline bool compatible(const GenericDrawCommand& other) const {
        return _queryID == other._queryID && _lodIndex == other._lodIndex &&
               _drawToBuffer == other._drawToBuffer &&
               _renderWireframe == other._renderWireframe &&
               _renderGeometry == other._renderGeometry &&
               _stateHash == other._stateHash && _type == other._type &&
               (_shaderProgram != nullptr) ==
                   (other._shaderProgram != nullptr) &&
               (_sourceBuffer != nullptr) == 
                   (other._sourceBuffer != nullptr);
    }
};

class TextureData {
    public:
    TextureData()
        : _textureType(TextureType::TEXTURE_2D),
          _textureHandle(0),
          _samplerHash(0)
    {
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

    TextureType _textureType;
    size_t _samplerHash;

private:
    U64  _textureHandle;
};

typedef vectorImpl<TextureData> TextureDataContainer;
typedef std::array<IndirectDrawCommand, Config::MAX_VISIBLE_NODES> DrawCommandList;

enum class ShaderType : U32;
class Shader;
class Texture;
class TextureData;
class IMPrimitive;
class PixelBuffer;
class Framebuffer;
class VertexBuffer;
class ShaderBuffer;
class ShaderProgram;

/// Renderer Programming Interface
class NOINITVTABLE RenderAPIWrapper {
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
    virtual void setCursorPosition(U16 x, U16 y) = 0;
    virtual IMPrimitive* newIMP() const = 0;
    virtual Framebuffer* newFB(bool multisampled) const = 0;
    virtual VertexBuffer* newVB() const = 0;
    virtual ShaderBuffer* newSB(const stringImpl& bufferName,
                                const bool unbound = false,
                                const bool persistentMapped = true) const = 0;
    virtual GenericVertexData* newGVD(
        const bool persistentMapped = false) const = 0;
    virtual PixelBuffer* newPB(const PBType& type = PBType::PB_TEXTURE_2D) const = 0;
    virtual Texture* newTextureArray() const = 0;
    virtual Texture* newTexture2D() const = 0;
    virtual Texture* newTextureCubemap() const = 0;
    virtual ShaderProgram* newShaderProgram() const = 0;
    virtual Shader* newShader(const stringImpl& name, const ShaderType& type,
                              const bool optimise = false) const = 0;
    virtual bool initShaders() = 0;
    virtual bool deInitShaders() = 0;

    virtual ErrorCode initRenderingAPI(I32 argc, char** argv) = 0;
    virtual void closeRenderingAPI() = 0;

    virtual void toggleRasterization(bool state) = 0;
    virtual void drawText(const TextLabel& textLabel,
                          const vec2<F32>& relativeOffset) = 0;

    virtual void updateClipPlanes() = 0;
    virtual U64 getFrameDurationGPU() = 0;
    virtual void activateStateBlock(const RenderStateBlock& newBlock,
                                    RenderStateBlock* const oldBlock) const = 0;

    virtual void drawPoints(U32 numPoints) = 0;

   protected:
    virtual void changeResolution(U16 w, U16 h) = 0;
    virtual void changeViewport(const vec4<I32>& newViewport) const = 0;
    virtual void threadedLoadCallback() = 0;
    virtual void uploadDrawCommands(const DrawCommandList& drawCommands,
                                    U32 commandCount) const = 0;

    virtual bool makeTexturesResident(const TextureDataContainer& textureData) = 0;
    virtual bool makeTextureResident(const TextureData& textureData) = 0;
};

};  // namespace Divide

#endif
