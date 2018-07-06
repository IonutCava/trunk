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
	void exitRenderLoop(bool killCommand = false); 
	void closeRenderingApi();
	void initDevice(U32 targetFrameRate);
	void changeResolution(U16 w, U16 h);
	///Change the window size without reshaping window data
	void setWindowSize(U16 w, U16 h);
	///Change the window's position
	void setWindowPos(U16 w, U16 h);
	void lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up = vec3<F32>(0,1,0), bool invertx = false, bool inverty = false);
	void idle();
	void flush();
	void getMatrix(MATRIX_MODE mode, mat4<F32>& mat);

	inline FrameBufferObject*  newFBO(FBOType type)  {
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

	inline VertexBufferObject* newVBO()                                             {return New d3dVertexBufferObject();}
	inline PixelBufferObject*  newPBO(PBOType type)                                 {return New d3dPixelBufferObject(type);}
	inline Texture2D*          newTexture2D(bool flipped = false)                   {return New d3dTexture(d3dTextureTypeTable[TEXTURE_2D]);}
	inline TextureCubemap*     newTextureCubemap(bool flipped = false)              {return New d3dTexture(d3dTextureTypeTable[TEXTURE_CUBE_MAP]);}
	inline ShaderProgram*      newShaderProgram()                                   {return New d3dShaderProgram();}
	inline Shader*             newShader(const std::string& name, ShaderType type)  {return New d3dShader(name,type);}
	
	void clearBuffers(U16 buffer_mask);
	void swapBuffers();
	void enableFog(F32 density, F32* color);
	
	void lockProjection();
	void releaseProjection();
	void lockModelView();
	void releaseModelView();
	void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes);
	void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes);

	void toggle2D(bool _2D);

	void drawTextToScreen(GUIElement* const);
	void drawButton(GUIElement* const);
	void drawFlash(GUIElement* const);

	void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset);
	void drawLines(const vectorImpl<vec3<F32> >& pointsA,const vectorImpl<vec3<F32> >& pointsB,const vectorImpl<vec4<F32> >& colors, const mat4<F32>& globalOffset);

	void renderModel(Object3D* const model);
	void renderElements(PrimitiveType t, GFXDataFormat f, U32 count, const void* first_element);

	void renderInViewport(const vec4<F32>& rect, boost::function0<void> callback);

	void setMaterial(Material* mat);

	friend class GFXDevice;
	typedef void (*callback)();
	void dxCommand(callback f){(*f)();};

	void setAmbientLight(const vec4<F32>& light){}
	void setLight(Light* const light){};

	void Screenshot(char *filename, const vec4<F32>& rect);
	RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor);
	void updateStateInternal(RenderStateBlock* block, bool force = false);
	void toggleDepthMapRendering(bool state){};

	void setObjectState(Transform* const transform, bool force = false, ShaderProgram* const shader = NULL);
	void releaseObjectState(Transform* const transform, ShaderProgram* const shader = NULL);


	F32 applyCropMatrix(frustum &f,SceneGraph* sceneGraph);
	bool loadInContext(const CurrentContext& context, boost::function0<void> callback);
END_SINGLETON

#endif