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

enum RENDER_STAGE {
    DEFERRED_STAGE   = 0x2,
    SHADOW_STAGE     = 0x4,
    REFLECTION_STAGE = 0x8,
    SSAO_STAGE       = 0x10,
    BLOOM_STAGE      = 0x20,
    FINAL_STAGE      = 0x40,
    DEPTH_STAGE      = 0x80,
	ENVIRONMENT_MAPPING_STAGE     = 0x160,
	//Place all stages above this
	INVALID_STAGE    = 0x400
};

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
	inline ShaderProgram* newShaderProgram(){return _api.newShaderProgram(); }
	inline Shader*        newShader(const std::string& name, SHADER_TYPE type) {return _api.newShader(name,type); }


	inline void clearBuffers(U8 buffer_mask){_api.clearBuffers(buffer_mask);}
	inline void swapBuffers(){_api.swapBuffers();}
	inline void enableFog(F32 density, F32* color){_api.enableFog(density,color);}

	inline void toggle2D(bool _2D) {_api.toggle2D(_2D);}

	inline void lockProjection(){_api.lockProjection();}
	inline void releaseProjection(){_api.releaseProjection();}
	inline void lockModelView(){_api.lockModelView();}
	inline void releaseModelView(){_api.releaseModelView();}
	inline void setOrthoProjection(const vec4& rect, const vec2& planes){_api.setOrthoProjection(rect,planes);}
	inline void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2& planes){_api.setPerspectiveProjection(FoV,aspectRatio,planes);}

	inline void drawTextToScreen(Text* text){_api.drawTextToScreen(text);}
	inline void drawCharacterToScreen(void* font,char character){_api.drawCharacterToScreen(font,character);}
	inline void drawButton(Button* button){_api.drawButton(button);}
	inline void drawFlash(GuiFlash* flash){_api.drawFlash(flash);}

	
	inline void drawBox3D(vec3 min, vec3 max){_api.drawBox3D(min,max);}

	void renderInViewport(const vec4& rect, boost::function0<void> callback);

	void renderModel(Object3D* const model);
	inline void renderElements(Type t, Format f, U32 count, const void* first_element){_api.renderElements(t,f,count,first_element);}
	
	inline void setMaterial(Material* mat){_api.setMaterial(mat);}

	void setAmbientLight(const vec4& light){_api.setAmbientLight(light);}
	inline void setLight(U8 slot, unordered_map<std::string,vec4>& properties_v,unordered_map<std::string,F32>& properties_f, LIGHT_TYPE type){_api.setLight(slot,properties_v,properties_f,type);}

	void toggleWireframe(bool state = false);
	inline void toggleDepthMapRendering(bool state) {_api.toggleDepthMapRendering(state);}
	void setRenderStage(RENDER_STAGE stage);
	inline RENDER_STAGE getRenderStage(){return _renderStage;}

	inline void         setDeferredRendering(bool state) {_deferredRendering = state;} 
	inline bool         getDeferredRendering() {return _deferredRendering;}

    inline bool wireframeRendering() {return _wireframeMode;}  
	inline void Screenshot(char *filename, const vec4& rect){_api.Screenshot(filename,rect);}

	inline void setPrevShaderId(const U32& id) {_prevShaderId = id;}
	inline const U32& getPrevShaderId() {return _prevShaderId;}

	inline void setPrevTextureId(const U32& id) {_prevTextureId = id;}
	inline const U32& getPrevTextureId() {return _prevTextureId;}

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
	F32 applyCropMatrix(frustum &f,SceneGraph* sceneGraph){return _api.applyCropMatrix(f,sceneGraph); }

	//Generate a cubemap from the given position
	//It renders the entire scene graph (with culling) as default
	//use the callback param to override the draw function
	void  generateCubeMap(FrameBufferObject& cubeMap, const vec3& pos, boost::function0<void> callback = 0);

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
		   _prevShaderId = 0;
		   _prevTextureId = 0;
		   _deferredRendering = false;
	   }
	RenderAPIWrapper& _api;
	bool _wireframeMode;
	bool _deferredRendering;
	RENDER_STAGE _renderStage;
	mat4 _currentLightProjectionMatrix;
    U32  _prevShaderId, _prevTextureId;
END_SINGLETON

#endif
