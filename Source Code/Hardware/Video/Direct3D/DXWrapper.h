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

DEFINE_SINGLETON_EXT1(DX_API,RenderAPIWrapper)

private:
	DX_API() : RenderAPIWrapper() {}

	void initHardware();
	void closeRenderingApi();
	void initDevice();
	void resizeWindow(U16 w, U16 h) {}
	void lookAt(const vec3& eye,const vec3& center,const vec3& up = vec3(0,1,0), bool invertx = false, bool inverty = false);
	void idle();

	void getModelViewMatrix(mat4& mvMat);
	void getProjectionMatrix(mat4& projMat);

	FrameBufferObject*  newFBO(){return /*new dxFrameBufferObject();*/ NULL; }
	VertexBufferObject* newVBO(){return /*new dxVertexBufferObject();*/ NULL; }
	PixelBufferObject*  newPBO(){return /*new dxPixelBufferObject();*/ NULL;}
	Texture2D*          newTexture2D(bool flipped = false){return /*new dxTexture2D();*/ NULL;}
	TextureCubemap*     newTextureCubemap(bool flipped = false){return /*new dxTextureCubemap();*/ NULL;}
	ShaderProgram*      newShaderProgram(){return /*new dxShaderProgram();*/ NULL;}
	Shader*             newShader(const std::string& name, SHADER_TYPE type) {return /*new dxShader(name,type);*/ NULL;}
	
	void clearBuffers(U8 buffer_mask);
	void swapBuffers();
	void enableFog(F32 density, F32* color);
	
	void lockProjection();
	void releaseProjection();
	void lockModelView();
	void releaseModelView();
	void setOrthoProjection(const vec4& rect, const vec2& planes);
	void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2& planes);

	void toggle2D(bool _2D);

	void drawTextToScreen(Text*);
	void drawCharacterToScreen(void* ,char);
	void drawButton(Button*);
	void drawFlash(GuiFlash* flash);

	void drawBox3D(vec3 min, vec3 max);

	void renderModel(Object3D* const model);
	void renderElements(Type t, Format f, U32 count, const void* first_element);

	void renderInViewport(const vec4& rect, boost::function0<void> callback);

	void setMaterial(Material* mat);

	friend class GFXDevice;
	typedef void (*callback)();
	void dxCommand(callback f){(*f)();};

	void setAmbientLight(const vec4& light){}
	void setLight(U8 slot, unordered_map<std::string,vec4>& properties_v,unordered_map<std::string,F32>& properties_f, LIGHT_TYPE type){};

	void toggleWireframe(bool state);
	void Screenshot(char *filename, const vec4& rect);
	void setRenderState(RenderState& state,bool force = false){}
	void ignoreStateChanges(bool state){}

	void toggleDepthMapRendering(bool state){};

	void setObjectState(Transform* const transform);
	void releaseObjectState(Transform* const transform);

	F32 applyCropMatrix(frustum &f,SceneGraph* sceneGraph);
END_SINGLETON

#endif