/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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
	void exitRenderLoop(const bool killCommand = false);
	void closeRenderingApi();
	void initDevice(U32 targetFrameRate);
	void changeResolution(U16 w, U16 h);
	///Change the window's position
	void setWindowPos(U16 w, U16 h);
	void lookAt(const vec3<F32>& eye,
                const vec3<F32>& center,
                const vec3<F32>& up = vec3<F32>(0,1,0),
                const bool invertx = false,
                const bool inverty = false);
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
    IMPrimitive* createPrimitive() { return NULL; }
	void renderInstance(RenderInstance* const instance);
	void renderBuffer(VertexBufferObject* const vbo, Transform* const vboTransform = NULL);

	void renderInViewport(const vec4<I32>& rect, boost::function0<void> callback);

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