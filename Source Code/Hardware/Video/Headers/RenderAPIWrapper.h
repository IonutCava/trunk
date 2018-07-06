/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _RENDER_API_H_
#define _RENDER_API_H_

#include "RenderAPIEnums.h"
#include "Utility/Headers/Vector.h"
#include "Core/Math/Headers/MathClasses.h"

class ShaderProgram;

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
//FWD DECLARE CLASSES

struct IndirectDrawCommand {
    IndirectDrawCommand() : count(0), instanceCount(1), firstIndex(0), baseVertex(0), baseInstance(0) {}
    U32  count;
    U32  instanceCount;
    U32  firstIndex;
    U32  baseVertex;
    U32  baseInstance;
};

struct GenericDrawCommand {
    U8     _queryID;
    U8     _lodIndex;
    bool   _drawToBuffer;
    I32    _drawID;
    size_t _stateHash;
    PrimitiveType _type;
    IndirectDrawCommand _cmd;
    ShaderProgram*      _shaderProgram;

    inline void setLoD(U8 lod)                 { _lodIndex = lod; }
    inline void setQueryID(U8 queryID)         { _queryID = queryID; }
    inline void setStateHash(size_t hashValue) { _stateHash = hashValue; }
    inline void setDrawToBuffer(bool state)    { _drawToBuffer = state; }
    inline void setInstanceCount(U32 count)    { _cmd.instanceCount = count; }
    inline void setDrawID(I32 drawID)          { _drawID = drawID; }

    inline void setShaderProgram(ShaderProgram* const program) { _shaderProgram = program; }

    GenericDrawCommand() : GenericDrawCommand(TRIANGLE_STRIP, 0, 0)
    {
    }

    GenericDrawCommand(const PrimitiveType& type, U32 firstIndex, U32 count, U32 instanceCount = 1) : _type(type),
                                                                                                      _lodIndex(0),
                                                                                                      _stateHash(0),
                                                                                                      _queryID(0),
                                                                                                      _drawID(-1),
                                                                                                      _drawToBuffer(false),
                                                                                                      _shaderProgram(nullptr)
    {
        _cmd.count = count;
        _cmd.firstIndex = firstIndex;
        _cmd.instanceCount = instanceCount;
    }
};

class Light;
class Shader;
class Kernel;
class SubMesh;
class Texture;
class Material;
class Object3D;
class TextLabel;
class Transform;
class SceneGraph;
class GUIElement;
class IMPrimitive;
class PixelBuffer;
class Framebuffer;
class VertexBuffer;
class ShaderBuffer;
class ShaderProgram;
class SceneGraphNode;
class SceneRenderState;
class RenderStateBlock;
class GenericVertexData;

///FWD DECLARE ENUMS
enum ShaderType;

template<class T> class Plane;
typedef vectorImpl<Plane<F32> > PlaneList;
///FWD DECLARE STRUCTS
class RenderStateBlockDescriptor;

///Renderer Programming Interface
class RenderAPIWrapper {
public: //RenderAPIWrapper global

protected:
    RenderAPIWrapper() : _apiId(GFX_RENDER_API_PLACEHOLDER),
                         _GPUVendor(GPU_VENDOR_PLACEHOLDER)
    {
    }

    friend class GFXDevice;

    inline void setId(const RenderAPI& apiId)                       {_apiId = apiId;}
    inline void setGPUVendor(const GPUVendor& gpuvendor)            {_GPUVendor = gpuvendor;}

    inline const RenderAPI&        getId()        const { return _apiId;}
    inline const GPUVendor&        getGPUVendor() const { return _GPUVendor;}

    /*Application display frame*/
    ///Clear buffers, set default states, etc
    virtual void beginFrame() = 0;
    ///Clear shaders, restore active texture units, etc
    virtual void endFrame() = 0;
    ///Clear buffers,shaders, etc.
    virtual void flush() = 0;

    virtual void idle() = 0;

    ///Change the window's position
    virtual void setWindowPos(U16 w, U16 h) const = 0;
    ///Platform specific cursor manipulation. Set's the cursor's location to the specified X and Y relative to the edge of the window
    virtual void setMousePosition(U16 x, U16 y) const = 0;
    virtual IMPrimitive*        newIMP() const = 0;
    virtual Framebuffer*        newFB(bool multisampled) const = 0;
    virtual VertexBuffer*       newVB() const = 0;
    virtual ShaderBuffer*       newSB(const bool unbound = false, const bool persistentMapped = true) const = 0;
    virtual GenericVertexData*  newGVD(const bool persistentMapped = false) const = 0;
    virtual PixelBuffer*        newPB(const PBType& type = PB_TEXTURE_2D) const = 0;
    virtual Texture*            newTextureArray(const bool flipped = false) const = 0;
    virtual Texture*            newTexture2D(const bool flipped = false) const = 0;
    virtual Texture*            newTextureCubemap(const bool flipped = false) const = 0;
    virtual ShaderProgram*      newShaderProgram(const bool optimise = false) const = 0;
    virtual Shader*             newShader(const std::string& name,const ShaderType& type,const bool optimise = false) const = 0;
    virtual bool                initShaders() = 0;
    virtual bool                deInitShaders() = 0;

    virtual I8   initHardware(const vec2<U16>& resolution, I32 argc, char **argv) = 0;
    virtual void exitRenderLoop(bool killCommand = false) = 0;
    virtual void closeRenderingApi() = 0;
    virtual void initDevice(U32 targetFrameRate) = 0;

    virtual void toggleRasterization(bool state) = 0;
    virtual void setLineWidth(F32 width) = 0;
    virtual void drawText(const TextLabel& textLabel, const vec2<I32>& position) = 0;

    /*Object viewing*/
    virtual void updateClipPlanes() = 0;
    /*Object viewing*/

    virtual ~RenderAPIWrapper(){};

    virtual U64 getFrameDurationGPU() const = 0;
    virtual void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock) const = 0;

    virtual void drawPoints(U32 numPoints) = 0;

protected:
    ///Change the resolution and reshape all graphics data
    virtual void changeResolutionInternal(U16 w, U16 h) = 0;
    virtual void changeViewport(const vec4<I32>& newViewport) const = 0;
    virtual void loadInContextInternal() = 0;

private:
    RenderAPI        _apiId;
    GPUVendor        _GPUVendor;
};

#endif
