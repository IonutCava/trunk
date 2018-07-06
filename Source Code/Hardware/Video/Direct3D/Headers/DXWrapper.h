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

#ifndef _WRAPPER_DX_H_
#define _WRAPPER_DX_H_

#include "Hardware/Video/Headers/RenderAPIWrapper.h"

#include "core.h"
#include "Hardware/Video/Direct3D/Buffers/FrameBuffer/Headers/d3dRenderTarget.h"
#include "Hardware/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dVertexBuffer.h"
#include "Hardware/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dGenericVertexData.h"
#include "Hardware/Video/Direct3D/Buffers/PixelBuffer/Headers/d3dPixelBuffer.h"
#include "Hardware/Video/Direct3D/Shaders/Headers/d3dShaderProgram.h"
#include "Hardware/Video/Direct3D/Shaders/Headers/d3dShader.h"
#include "Hardware/Video/Direct3D/Textures/Headers/d3dTexture.h"
#include "Hardware/Video/Direct3D/Headers/d3dEnumTable.h"

DEFINE_SINGLETON_EXT1(DX_API,RenderAPIWrapper)

protected:
    DX_API() : RenderAPIWrapper() {}

    I8   initHardware(const vec2<U16>& resolution, I32 argc, char **argv);
    void exitRenderLoop(bool killCommand = false);
    void closeRenderingApi();
    void initDevice(U32 targetFrameRate);
    void changeResolutionInternal(U16 w, U16 h);
    void setMousePosition(U16 x, U16 y) const;
    ///Change the window's position
    void setWindowPos(U16 w, U16 h)  const;
    vec3<F32> unproject(const vec3<F32>& windowCoord) const;
    F32* lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& viewDirection);
    F32* lookAt(const vec3<F32>& eye, const vec3<F32>& target, const vec3<F32>& up);
    const F32* getLookAt(const vec3<F32>& eye, const vec3<F32>& target, const vec3<F32>& up);
    void idle();
    void flush();
    void beginFrame();
    void endFrame();
    void clearStates(const bool skipShader,const bool skipTextures,const bool skipBuffers, const bool forceAll) {}
    void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat);

    inline FrameBuffer*        newFB(bool multisampled)                       {return New d3dRenderTarget(multisampled); }
    inline GenericVertexData*  newGVD()                                       {return New d3dGenericVertexData(); }
    inline VertexBuffer*       newVB(const PrimitiveType& type)               {return New d3dVertexBuffer(type);}
    inline PixelBuffer*        newPB(const PBType& type)                      {return New d3dPixelBuffer(type);}
    inline Texture2D*          newTexture2D(const bool flipped = false)       {return New d3dTexture(d3dTextureTypeTable[TEXTURE_2D]);}
    inline TextureCubemap*     newTextureCubemap(const bool flipped = false)  {return New d3dTexture(d3dTextureTypeTable[TEXTURE_CUBE_MAP]);}
    inline ShaderProgram*      newShaderProgram(const bool optimise = false)  {return New d3dShaderProgram(optimise);}

    inline Shader*             newShader(const std::string& name,const  ShaderType& type,const bool optimise = false)  {return New d3dShader(name,type,optimise);}
           bool                initShaders();
           bool                deInitShaders();

    void lockMatrices(const MATRIX_MODE& setCurrentMatrix, bool lockView = true, bool lockProjection = true);
    void releaseMatrices(const MATRIX_MODE& setCurrentMatrix, bool releaseView = true, bool releaseProjection = true);
    F32* setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes);
    const F32* getOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes);
    F32* setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes);
    void setAnaglyphFrustum(F32 camIOD, bool rightFrustum = false);

    void toggle2D(bool _2D);

    void drawText(const TextLabel& textLabel, const vec2<I32>& position);
    void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset);
    void drawLines(const vectorImpl<vec3<F32> >& pointsA,
                   const vectorImpl<vec3<F32> >& pointsB,
                   const vectorImpl<vec4<U8> >& colors,
                   const mat4<F32>& globalOffset,
                   const bool orthoMode = false,
                   const bool disableDepth = false);
    void debugDraw();
    IMPrimitive* createPrimitive(bool allowPrimitiveRecycle = false) { return nullptr; }

    void renderInViewport(const vec4<I32>& rect, const DELEGATE_CBK& callback);
    void updateClipPlanes();
    friend class GFXDevice;
    typedef void (*callback)();
    void dxCommand(callback f){(*f)();};

    void setLight(Light* const light){};

    void Screenshot(char *filename, const vec4<F32>& rect);

    bool loadInContext(const CurrentContext& context, const DELEGATE_CBK& callback);

    U64 getFrameDurationGPU() const { return 0; }
    I32 getDrawCallCount()    const { return 0; }

    void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock);

END_SINGLETON

#endif