/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#ifndef _GFX_DEVICE_H
#define _GFX_DEVICE_H

#include "OpenGL\GLWrapper.h"
#include "Direct3D\DXWrapper.h"

class Object3D;
class Framerate;


//Rough around the edges Adapter pattern
DEFINE_SINGLETON_EXT1(GFXDevice,RenderAPIWrapper)
friend class Frustum; //For matrix recovery operations
public:
	void setApi(RenderAPI api);
	I8  getApi(){return _api.getId(); }
	void initHardware(){_api.initHardware();}
	void initDevice(){_api.initDevice();}
	void resizeWindow(U16 w, U16 h);
	inline void lookAt(const vec3& eye,const vec3& center,const vec3& up = vec3(0,1,0), bool invertx = false, bool inverty = false){_api.lookAt(eye,center,up,invertx,inverty);}
	inline void idle() {_api.idle();}

	inline mat4& getLightProjectionMatrix() {return _currentLightProjectionMatrix;}
	inline void setLightProjectionMatrix(const mat4& lightMatrix){_currentLightProjectionMatrix = lightMatrix;}

	inline void closeRenderingApi(){_api.closeRenderingApi();}
	inline FrameBufferObject* newFBO(){return _api.newFBO(); }
	inline VertexBufferObject* newVBO(){return _api.newVBO(); }
	inline PixelBufferObject*  newPBO(){return _api.newPBO(); }

	inline Texture2D*          newTexture2D(bool flipped = false){return _api.newTexture2D(flipped);}
	inline TextureCubemap*     newTextureCubemap(bool flipped = false){return _api.newTextureCubemap(flipped);}

	inline Shader* newShader(const char *vsFile, const char *fsFile){return _api.newShader(vsFile,fsFile); }
	inline Shader* newShader(){return _api.newShader(); }


	inline void clearBuffers(U8 buffer_mask){_api.clearBuffers(buffer_mask);}
	inline void swapBuffers(){_api.swapBuffers();}
	inline void enableFog(F32 density, F32* color){_api.enableFog(density,color);}

	inline void toggle2D(bool _2D) {_api.toggle2D(_2D);}

	inline void lockProjection(){_api.lockProjection();}
	inline void releaseProjection(){_api.releaseProjection();}
	inline void lockModelView(){_api.lockModelView();}
	inline void releaseModelView(){_api.releaseModelView();}
	inline void setOrthoProjection(const vec4& rect, const vec2& planes){_api.setOrthoProjection(rect,planes);}

	inline void drawTextToScreen(Text* text){_api.drawTextToScreen(text);}
	inline void drawCharacterToScreen(void* font,char character){_api.drawCharacterToScreen(font,character);}
	inline void drawButton(Button* button){_api.drawButton(button);}
	inline void drawFlash(GuiFlash* flash){_api.drawFlash(flash);}

	
	inline void drawBox3D(vec3 min, vec3 max){_api.drawBox3D(min,max);}

	void renderModel(Object3D* const model);
	inline void renderElements(Type t, Format f, U32 count, const void* first_element){_api.renderElements(t,f,count,first_element);}
	
	inline void setMaterial(Material* mat){_api.setMaterial(mat);}

	void setAmbientLight(const vec4& light){_api.setAmbientLight(light);}
	inline void setLight(U8 slot, unordered_map<std::string,vec4>& properties_v,unordered_map<std::string,F32>& properties_f, LIGHT_TYPE type){_api.setLight(slot,properties_v,properties_f,type);}

	void toggleWireframe(bool state = false);
    void setDepthMapRendering(bool state) {_api.setDepthMapRendering(state);_depthMapRendering = state;}
    inline bool getDepthMapRendering() {return _depthMapRendering;}

    inline void setDeferredShading(bool state) {_deferredShading = state;}
    inline bool getDeferredShading() {return _deferredShading;}
    inline void setSSAOShading(bool state) {_ssaoShading = state;}
    inline bool getSSAOShading() {return _ssaoShading;}

    inline bool wireframeRendering() {return _wireframeMode;}  
	inline void Screenshot(char *filename, const vec4& rect){_api.Screenshot(filename,rect);}

	inline void setPrevShaderId(const U32& id) {_prevShaderId = id;}
	inline const U32& getPrevShaderId() {return _prevShaderId;}

	inline void setPrevTextureId(const U32& id) {_prevTextureId = id;}
	inline const U32& getPrevTextureId() {return _prevTextureId;}

	inline void setPrevMaterialId(const I32& id) {_prevMaterialId = id;}
	inline const I32& getPrevMaterialId() {return _prevMaterialId;}
	void processRenderQueue();


    void setRenderState(RenderState& state,bool force = false);
	inline void restoreRenderState() {
		_api.setRenderState(_previousRenderState);
		
	}

    inline void ignoreStateChanges(bool state) {
		_api.ignoreStateChanges(state);
	}

	
	void setObjectState(Transform* const transform){_api.setObjectState(transform); }
	void releaseObjectState(Transform* const transform){_api.releaseObjectState(transform); }

public:
	enum BufferType
	{
		COLOR_BUFFER   = 0x0001,
		DEPTH_BUFFER   = 0x0010,
		STENCIL_BUFFER = 0x0100
	};
private:
	inline void getModelViewMatrix(mat4& mvMat){_api.getModelViewMatrix(mvMat);}
	inline void getProjectionMatrix(mat4& projMat){_api.getProjectionMatrix(projMat);}

private:
	GFXDevice() :
	   _api(GL_API::getInstance()) //Defaulting to OpenGL if no api has been defined
	   {
		   _wireframeMode = false;
		   _depthMapRendering = false;
		   _deferredShading = false;
		   _ssaoShading = false;
		   _prevShaderId = 0;
		   _prevTextureId = 0;
		   _prevMaterialId = 0;
	   }
	RenderAPIWrapper& _api;
	bool _wireframeMode,_deferredShading,_depthMapRendering,_ssaoShading;
	mat4 _currentLightProjectionMatrix;
    U32  _prevShaderId, _prevTextureId;
	I32  _prevMaterialId;
END_SINGLETON

#endif
