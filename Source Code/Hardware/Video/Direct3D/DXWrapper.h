/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#include "../RenderAPIWrapper.h"

#include "resource.h"
#include "d3dFrameBufferObject.h"
#include "d3dVertexBufferObject.h"
#include "d3dPixelBufferObject.h"
#include "d3dShaderProgram.h"
#include "d3dShader.h"
#include "d3dTexture.h"
#include "d3dEnumTable.h"

DEFINE_SINGLETON_EXT1(DX_API,RenderAPIWrapper)

private:
	DX_API() : RenderAPIWrapper() {}

	void initHardware();
	void closeRenderingApi();
	void initDevice();
	void resizeWindow(U16 w, U16 h) {}
	void lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up = vec3<F32>(0,1,0), bool invertx = false, bool inverty = false);
	void idle();

	void getModelViewMatrix(mat4<F32>& mvMat);
	void getProjectionMatrix(mat4<F32>& projMat);

	FrameBufferObject*  newFBO(){return New d3dFrameBufferObject();}
	VertexBufferObject* newVBO(){return New d3dVertexBufferObject();}
	PixelBufferObject*  newPBO(){return New d3dPixelBufferObject();}

	Texture2D*          newTexture2D(bool flipped = false){return New d3dTexture(d3dTextureTypeTable[TEXTURE_2D]);}
	TextureCubemap*     newTextureCubemap(bool flipped = false){return New d3dTexture(d3dTextureTypeTable[TEXTURE_CUBE_MAP]);}
	ShaderProgram*      newShaderProgram(){return New d3dShaderProgram();}
	Shader*             newShader(const std::string& name, SHADER_TYPE type) {return New d3dShader(name,type);}
	
	void clearBuffers(U8 buffer_mask);
	void swapBuffers();
	void enableFog(F32 density, F32* color);
	
	void lockProjection();
	void releaseProjection();
	void lockModelView();
	void releaseModelView();
	void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes);
	void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes);

	void toggle2D(bool _2D);

	void drawTextToScreen(GuiElement* const);
	void drawCharacterToScreen(void* ,char);
	void drawButton(GuiElement* const);
	void drawFlash(GuiElement* const);

	void drawBox3D(vec3<F32> min, vec3<F32> max);

	void renderModel(Object3D* const model);
	void renderElements(PRIMITIVE_TYPE t, VERTEX_DATA_FORMAT f, U32 count, const void* first_element);

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
END_SINGLETON

#endif