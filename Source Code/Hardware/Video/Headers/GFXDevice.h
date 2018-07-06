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

#ifndef _GFX_DEVICE_H
#define _GFX_DEVICE_H

#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
#include "Hardware/Video/Direct3D/Headers/DXWrapper.h"

enum RENDER_STAGE;
enum SCENE_NODE_TYPE;

class Light;
class Camera;
class Object3D;
class Framerate;

///Rough around the edges Adapter pattern
DEFINE_SINGLETON_EXT1(GFXDevice,RenderAPIWrapper)
friend class Frustum; ///< For matrix recovery operations

public:
	void setApi(RENDER_API api);

	inline RENDER_API         getApi()        {return _api.getId(); }
	inline RENDER_API_VERSION getApiVersion() {return _api.getVersionId();}

	inline void registerKernel(Kernel* const kernel)     {_kernel = kernel;}
	inline I8   initHardware(const vec2<U16>& resolution){return _api.initHardware(resolution);}
	inline void initDevice(U32 targetFPS)                {_api.initDevice(targetFPS);}
	inline void changeResolution(U16 w, U16 h)           {_api.changeResolution(w,h);}
	inline void exitRenderLoop(bool killCommand = false) {_api.exitRenderLoop(killCommand);}
	       void closeRenderingApi();

	inline void lookAt(const vec3<F32>& eye,
					   const vec3<F32>& center,
					   const vec3<F32>& up = vec3<F32>(0,1,0),
					   bool invertx = false,
					   bool inverty = false)
	{
	   _api.lookAt(eye,center,up,invertx,inverty);
	}

	inline void idle() {_api.idle();}

	inline mat4<F32>& getLightProjectionMatrix()                            {return _currentLightProjectionMatrix;}
	inline void       setLightProjectionMatrix(const mat4<F32>& lightMatrix){_currentLightProjectionMatrix = lightMatrix;}

	inline FrameBufferObject*  newFBO(){return _api.newFBO(); }
	inline VertexBufferObject* newVBO(){return _api.newVBO(); }
	inline PixelBufferObject*  newPBO(){return _api.newPBO(); }

	inline Texture2D*      newTexture2D(bool flipped = false)                   {return _api.newTexture2D(flipped);}
	inline TextureCubemap* newTextureCubemap(bool flipped = false)              {return _api.newTextureCubemap(flipped);}
	inline ShaderProgram*  newShaderProgram()                                   {return _api.newShaderProgram(); }
	inline Shader*         newShader(const std::string& name, SHADER_TYPE type) {return _api.newShader(name,type); }
	
	inline void enableFog(F32 density, F32* color){_api.enableFog(density,color);}

	inline void swapBuffers()               {_api.swapBuffers();}
	inline void clearBuffers(U8 buffer_mask){_api.clearBuffers(buffer_mask);}
	
	inline void toggle2D(bool _2D)  {_api.toggle2D(_2D);}
	inline void lockProjection()    {_api.lockProjection();}
	inline void releaseProjection() {_api.releaseProjection();}
	inline void lockModelView()     {_api.lockModelView();}
	inline void releaseModelView()  {_api.releaseModelView();}

	inline void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes)        {_api.setOrthoProjection(rect,planes);}
	inline void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes){_api.setPerspectiveProjection(FoV,aspectRatio,planes);}

	void renderInViewport(const vec4<F32>& rect, boost::function0<void> callback);

	void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset);
	void drawLines(const std::vector<vec3<F32> >& pointsA,const std::vector<vec3<F32> >& pointsB,const std::vector<vec4<F32> >& colors, const mat4<F32>& globalOffset);

	void renderModel(Object3D* const model);
	void renderElements(PRIMITIVE_TYPE t, VERTEX_DATA_FORMAT f, U32 count, const void* first_element);
	void renderGUIElement(GUIElement* const guiElement);

	///Set the current material properties
	inline void setMaterial(Material* mat)             {_api.setMaterial(mat);}
	///Instruct the Rendering API to modify the ambient light
	inline void setAmbientLight(const vec4<F32>& light){_api.setAmbientLight(light);}
	///Update light properties internally in the Rendering API
	inline void setLight(Light* const light)           {_api.setLight(light);}
	///Set internal API states for proper depthmap rendering (rember to use RenderStateBlocks for objects as well
	inline void toggleDepthMapRendering(bool state)    {_api.toggleDepthMapRendering(state);}

	///Sets the current render state.
	///@param state Is used to inform the rendering pipeline what we are rendering. Shadows? reflections? etc
		   void         setRenderStage(RENDER_STAGE stage);
	inline RENDER_STAGE getRenderStage()                 {return _renderStage;}
	inline void         setDeferredRendering(bool state) {_deferredRendering = state;} 
	inline bool         getDeferredRendering()           {return _deferredRendering;}
	inline void         setPrevShaderId(const U32& id)   {_prevShaderId = id;}
	inline U32          getPrevShaderId()                {return _prevShaderId;}
	inline void         setPrevTextureId(const U32& id)  {_prevTextureId = id;}
	inline U32          getPrevTextureId()               {return _prevTextureId;}
	///Save a screenshot in TGA format
	inline void Screenshot(char *filename, const vec4<F32>& rect){_api.Screenshot(filename,rect);}
	/// Get all items from the renderQueue, update their states and set materials
	void processRenderQueue();
	/// Some Scene Node Types are excluded from certain operations (lights triggers, etc)
	bool excludeFromStateChange(SCENE_NODE_TYPE currentType);
	///Creates a new API dependend stateblock based on the received description
	///Calls newRenderStateBlock and also saves the new block in the state block map
	RenderStateBlock* GFXDevice::createStateBlock(const RenderStateBlockDescriptor& descriptor);
	
private:
	///Returns an API dependend stateblock based on the description
	inline RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor) {
		return _api.newRenderStateBlock(descriptor);
	}
	/// Delegate specifig GUI drawing functionality to the Rendering API
	inline void drawTextToScreen(GUIElement* const text)        {_api.drawTextToScreen(text);}
	inline void drawCharacterToScreen(void* font,char character){_api.drawCharacterToScreen(font,character);}
	inline void drawButton(GUIElement* const button)            {_api.drawButton(button);}
	inline void drawFlash(GUIElement* const flash)              {_api.drawFlash(flash);}
	inline void drawConsole()                                   {_api.drawConsole();}

public:
	///Sets the current state block to the one passed as a param
	///It is not set immediately, but a call to "updateStates" is required
           void setStateBlock(RenderStateBlock* block);
	///Sets a standard state block
	inline void setDefaultStateBlock() {assert(_defaultStateBlock != NULL); setStateBlock(_defaultStateBlock);}
	///Same as setStateBlock(RenderStateBlock* block), but uses a blank description
		   void setStateBlockByDesc(const RenderStateBlockDescriptor &desc);
	///If a new state has been set, update the Graphics pipeline
		   void updateStates(bool force = false);
	///Update the graphics pipeline using the current rendering API with the state block passed
	inline void updateStateInternal(RenderStateBlock* block, bool force = false) {_api.updateStateInternal(block,force);}
	/*//Render State Management */

	inline void setObjectState(Transform* const transform, bool force = false, ShaderProgram* const shader = NULL){_api.setObjectState(transform,force,shader); }
	inline void releaseObjectState(Transform* const transform, ShaderProgram* const shader = NULL){_api.releaseObjectState(transform,shader); }
	inline F32 applyCropMatrix(frustum &f,SceneGraph* sceneGraph){return _api.applyCropMatrix(f,sceneGraph); }

	///Generate a cubemap from the given position
	///It renders the entire scene graph (with culling) as default
	///use the callback param to override the draw function
	void  generateCubeMap(FrameBufferObject& cubeMap, 
						  Camera* const activeCamera,
						  const vec3<F32>& pos, 
						  boost::function0<void> callback = 0);

	inline U16 getLastBinSize() {return _renderBinCount;}

public:
	enum BufferType	{
		COLOR_BUFFER   = 0x0001,
		DEPTH_BUFFER   = 0x0010,
		STENCIL_BUFFER = 0x0100
	};

private:
	inline void getModelViewMatrix(mat4<F32>& mvMat)   {_api.getModelViewMatrix(mvMat);}
	inline void getProjectionMatrix(mat4<F32>& projMat){_api.getProjectionMatrix(projMat);}

private:
	GFXDevice() :
	   _api(GL_API::getInstance()) ///<Defaulting to OpenGL if no api has been defined
	   {
		   _prevShaderId = 0;
		   _prevTextureId = 0;
		   _deferredRendering = false;
		   _currentStateBlock = NULL;
		   _newStateBlock = NULL;
		   _stateBlockDirty = false;
	   }
	RenderAPIWrapper& _api;
	bool _deferredRendering;
	bool _deviceStateDirty;
	RENDER_STAGE _renderStage;
	mat4<F32> _currentLightProjectionMatrix;
    U32  _prevShaderId, _prevTextureId;
	U16 _renderBinCount;

protected:
	friend class GL_API;
	friend class DX_API;
	/*State management */
	typedef unordered_map<U32, RenderStateBlock* > RenderStateMap;
	RenderStateMap _stateBlockMap;
    bool  _stateBlockDirty;
	bool _stateBlockByDescription;
	RenderStateBlock* _currentStateBlock;
    RenderStateBlock* _newStateBlock;
	RenderStateBlock* _defaultStateBlock;
	///Pointer to current kernel
	Kernel* _kernel;
END_SINGLETON

#define GFX_DEVICE GFXDevice::getInstance()
#define GFX_RENDER_BIN_SIZE GFX_DEVICE.getLastBinSize()

#endif
