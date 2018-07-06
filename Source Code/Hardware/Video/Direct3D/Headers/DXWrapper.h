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
#include "Hardware/Video/Direct3D/Buffers/Framebuffer/Headers/d3dRenderTarget.h"
#include "Hardware/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dVertexBuffer.h"
#include "Hardware/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dGenericVertexData.h"
#include "Hardware/Video/Direct3D/Buffers/ShaderBuffer/Headers/d3dConstantBuffer.h"
#include "Hardware/Video/Direct3D/Buffers/PixelBuffer/Headers/d3dPixelBuffer.h"
#include "Hardware/Video/Direct3D/Shaders/Headers/d3dShaderProgram.h"
#include "Hardware/Video/Direct3D/Shaders/Headers/d3dShader.h"
#include "Hardware/Video/Direct3D/Textures/Headers/d3dTexture.h"
#include "Hardware/Video/Direct3D/Headers/d3dEnumTable.h"

namespace Divide {

DEFINE_SINGLETON_EXT1(DX_API, RenderAPIWrapper)

protected:
    DX_API() : RenderAPIWrapper() {}

    ErrorCode initRenderingApi(const vec2<U16>& resolution, I32 argc, char **argv);
    void      closeRenderingApi();
    void changeResolutionInternal(U16 w, U16 h);
    void changeViewport(const vec4<I32>& newViewport) const;
    void setCursorPosition(U16 x, U16 y) const;
    ///Change the window's position
    void setWindowPos(U16 w, U16 h)  const;
    void beginFrame();
    void endFrame();

    inline ShaderBuffer*       newSB(const bool unbound = false, const bool persistentMapped = true) const { return New d3dConstantBuffer(unbound, persistentMapped); }

    inline IMPrimitive*        newIMP()                                      const { return nullptr; }
    inline Framebuffer*        newFB(bool multisampled)                      const { return New d3dRenderTarget(multisampled); }
    inline GenericVertexData*  newGVD(const bool persistentMapped = false)   const { return New d3dGenericVertexData(persistentMapped); }
    inline VertexBuffer*       newVB()                                       const { return New d3dVertexBuffer(); }
    inline PixelBuffer*        newPB(const PBType& type)                     const { return New d3dPixelBuffer(type); }
    inline Texture*            newTextureArray(const bool flipped = false)   const { return New d3dTexture(TEXTURE_2D_ARRAY, flipped); }
    inline Texture*            newTexture2D(const bool flipped = false)      const { return New d3dTexture(TEXTURE_2D, flipped); }
    inline Texture*            newTextureCubemap(const bool flipped = false) const { return New d3dTexture(TEXTURE_CUBE_MAP, flipped); }
    inline ShaderProgram*      newShaderProgram(const bool optimise = false) const { return New d3dShaderProgram(optimise); }

    inline Shader*             newShader(const std::string& name,const  ShaderType& type,const bool optimise = false) const {return New d3dShader(name,type,optimise);}
           bool                initShaders();
           bool                deInitShaders();

    F32* setProjection(const vec4<F32>& rect, const vec2<F32>& planes);
    F32* setProjection(F32 FoV, F32 aspectRatio, const vec2<F32>& planes);
    void setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes, F32 aspectRatio, F32 verticalFoV, bool rightFrustum = false);

    void toggleRasterization(bool state);
    void setLineWidth(F32 width);

    void drawText(const TextLabel& textLabel, const vec2<I32>& position);
    void drawPoints(U32 numPoints);

    void updateClipPlanes();
    friend class GFXDevice;
    typedef void (*callback)();
    void dxCommand(callback f){(*f)();};

    void createLoaderThread();

    U64 getFrameDurationGPU() { return 0; }

    void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock) const;

END_SINGLETON

}; //namespace Divide
#endif