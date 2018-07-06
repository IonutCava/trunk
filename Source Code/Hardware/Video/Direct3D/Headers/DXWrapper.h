/*
   Copyright (c) 2013 DIVIDE-Studio
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
#include "Hardware/Video/Direct3D/Buffers/FrameBufferObject/Headers/d3dTextureBufferObject.h"
#include "Hardware/Video/Direct3D/Buffers/FrameBufferObject/Headers/d3dDeferredBufferObject.h"
#include "Hardware/Video/Direct3D/Buffers/FrameBufferObject/Headers/d3dDepthBufferObject.h"
#include "Hardware/Video/Direct3D/Buffers/VertexBufferObject/Headers/d3dVertexBufferObject.h"
#include "Hardware/Video/Direct3D/Buffers/PixelBufferObject/Headers/d3dPixelBufferObject.h"
#include "Hardware/Video/Direct3D/Shaders/Headers/d3dShaderProgram.h"
#include "Hardware/Video/Direct3D/Shaders/Headers/d3dShader.h"
#include "Hardware/Video/Direct3D/Textures/Headers/d3dTexture.h"
#include "Hardware/Video/Direct3D/Headers/d3dEnumTable.h"

DEFINE_SINGLETON_EXT1(DX_API,RenderAPIWrapper)

private:
    DX_API() : RenderAPIWrapper() {}

    I8   initHardware(const vec2<U16>& resolution, I32 argc, char **argv);
    void exitRenderLoop(bool killCommand = false);
    void closeRenderingApi();
    void initDevice(U32 targetFrameRate);
    void changeResolution(U16 w, U16 h);
    void setMousePosition(D32 x, D32 y) const;
    ///Change the window's position
    void setWindowPos(U16 w, U16 h)  const;
    vec3<F32> unproject(const vec3<F32>& windowCoord) const;
    void lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& viewDirection);
    void lookAt(const vec3<F32>& eye, const vec3<F32>& target, const vec3<F32>& up);
    void idle();
    void flush();
    void beginFrame();
    void endFrame();
    void clearStates(const bool skipShader,const bool skipTextures,const bool skipBuffers, const bool forceAll) {}
    void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat);
    void getMatrix(const EXTENDED_MATRIX& mode, mat4<F32>& mat);
    void getMatrix(const EXTENDED_MATRIX& mode, mat3<F32>& mat);

    inline FrameBufferObject*  newFBO(const FBOType& type)  {
        switch(type){
            case FBO_2D_DEFERRED:
                return New d3dDeferredBufferObject();
            case FBO_2D_DEPTH:
                return New d3dDepthBufferObject();
            case FBO_CUBE_COLOR:
                return New d3dTextureBufferObject(true);
            default:
            case FBO_2D_COLOR:
                return New d3dTextureBufferObject();
        }
    }

    inline VertexBufferObject* newVBO(const PrimitiveType& type)                    {return New d3dVertexBufferObject(type);}
    inline PixelBufferObject*  newPBO(const PBOType& type)                          {return New d3dPixelBufferObject(type);}
    inline Texture2D*          newTexture2D(const bool flipped = false)             {return New d3dTexture(d3dTextureTypeTable[TEXTURE_2D]);}
    inline TextureCubemap*     newTextureCubemap(const bool flipped = false)        {return New d3dTexture(d3dTextureTypeTable[TEXTURE_CUBE_MAP]);}
    inline ShaderProgram*      newShaderProgram(const bool optimise = false)        {return New d3dShaderProgram(optimise);}

    inline Shader*             newShader(const std::string& name,const  ShaderType& type,const bool optimise = false)  {return New d3dShader(name,type,optimise);}
           bool                initShaders();
           bool                deInitShaders();

    void lockMatrices(const MATRIX_MODE& setCurrentMatrix, bool lockView = true, bool lockProjection = true);
    void releaseMatrices(const MATRIX_MODE& setCurrentMatrix, bool releaseView = true, bool releaseProjection = true);
    void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes);
    void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes);
    void setAnaglyphFrustum(F32 camIOD, bool rightFrustum = false);

    void toggle2D(bool _2D);

    void drawText(const std::string& text, const I32 width, const std::string& fontName, const F32 fontSize);
    void drawText(const std::string& text, const I32 width, const vec2<I32> position, const std::string& fontName, const F32 fontSize);
    void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset);
    void drawLines(const vectorImpl<vec3<F32> >& pointsA,
                   const vectorImpl<vec3<F32> >& pointsB,
                   const vectorImpl<vec4<U8> >& colors,
                   const mat4<F32>& globalOffset,
                   const bool orthoMode = false,
                   const bool disableDepth = false);
    void debugDraw();
    IMPrimitive* createPrimitive(bool allowPrimitiveRecycle = false) { return NULL; }
    void renderInstance(RenderInstance* const instance);
    void renderBuffer(VertexBufferObject* const vbo, Transform* const vboTransform = NULL);

    void renderInViewport(const vec4<U32>& rect, boost::function0<void> callback);
    void updateClipPlanes();
    friend class GFXDevice;
    typedef void (*callback)();
    void dxCommand(callback f){(*f)();};

    void setLight(Light* const light){};

    void Screenshot(char *filename, const vec4<F32>& rect);
    RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor);
    void updateStateInternal(RenderStateBlock* block, bool force = false);

    bool loadInContext(const CurrentContext& context, boost::function0<void> callback);

END_SINGLETON

#endif