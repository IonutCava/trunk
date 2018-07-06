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

#ifndef _GFX_DEVICE_H
#define _GFX_DEVICE_H

#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
#include "Hardware/Video/Direct3D/Headers/DXWrapper.h"
#include "Managers/Headers/RenderPassManager.h" ///<For GFX_RENDER_BIN_SIZE
enum RenderStage;
enum SceneNodeType;

class Light;
class Camera;
class Object3D;
class Framerate;
class Renderer;
class SceneRenderState;
///Rough around the edges Adapter pattern
DEFINE_SINGLETON_EXT1(GFXDevice,RenderAPIWrapper)
friend class Frustum; ///< For matrix recovery operations

public:

	void setApi(RenderAPI api);

	inline RenderAPI        getApi()        {return _api.getId(); }
	inline RenderAPIVersion getApiVersion() {return _api.getVersionId();}
	inline GPUVendor        getGPUVendor()  {return _api.getGPUVendor();}

	inline I8   initHardware(const vec2<U16>& resolution, I32 argc, char **argv){return _api.initHardware(resolution,argc,argv);}

	inline void registerKernel(Kernel* const kernel)     {_kernel = kernel;}
	inline void initDevice(U32 targetFrameRate)          {_api.initDevice(targetFrameRate);}
	inline void changeResolution(U16 w, U16 h)           {_api.changeResolution(w,h);}
	inline void setWindowPos(U16 w, U16 h)               {_api.setWindowPos(w,h);}

	inline void exitRenderLoop(const bool killCommand = false) {_api.exitRenderLoop(killCommand);}
	       void closeRenderingApi();

	inline void lookAt(const vec3<F32>& eye,
					   const vec3<F32>& center,
					   const vec3<F32>& up = vec3<F32>(0,1,0),
					   const bool invertx = false,
					   const bool inverty = false)
	{
	   _api.lookAt(eye,center,up,invertx,inverty);
	}

	inline void beginFrame() {_api.beginFrame();}
	inline void endFrame()   {_api.endFrame();  }
	inline void idle()       {_api.idle();}
	inline void flush()      {_api.flush();}

	inline FrameBufferObject*  newFBO(const FBOType& type = FBO_2D_COLOR){return _api.newFBO(type); }
	inline VertexBufferObject* newVBO(const PrimitiveType& type = TRIANGLES){return _api.newVBO(type); }
	inline PixelBufferObject*  newPBO(const PBOType& type = PBO_TEXTURE_2D){return _api.newPBO(type); }

	inline Texture2D*      newTexture2D(const bool flipped = false)                   {return _api.newTexture2D(flipped);}
	inline TextureCubemap* newTextureCubemap(const bool flipped = false)              {return _api.newTextureCubemap(flipped);}
	inline ShaderProgram*  newShaderProgram(const bool optimise = false)              {return _api.newShaderProgram(optimise); }

	inline Shader*         newShader(const std::string& name,const  ShaderType& type,const bool optimise = false)  {return _api.newShader(name,type,optimise); }
    ///Hardware specific shader preps (e.g.: OpenGL: init/deinit GLSL-OPT and GLSW)
    inline bool            initShaders()                                        {return _api.initShaders();}
    inline bool            deInitShaders()                                      {return _api.deInitShaders();}

	void enableFog(FogMode mode, F32 density, const vec3<F32>& color, F32 startDist, F32 endDist);

	inline void toggle2D(bool _2D)  {_api.toggle2D(_2D);}
    ///Usually, after locking and releasing our matrices we want to revert to the View matrix to render geometry
    inline void lockMatrices(const MATRIX_MODE& setCurrentMatrix = VIEW_MATRIX, bool lockView = true, bool lockProjection = true)          {_api.lockMatrices(setCurrentMatrix,lockView,lockProjection);}
    inline void releaseMatrices(const MATRIX_MODE& setCurrentMatrix = VIEW_MATRIX, bool releaseView = true, bool releaseProjection = true) {_api.releaseMatrices(setCurrentMatrix,releaseView,releaseProjection);}
            ///sets an ortho projection, updating any listeners if needed
            void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes);
            ///sets a perspective projection, updating any listeners if needed
	        void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes);
			///sets a new horizontal FoV
			void setHorizontalFoV(I32 newFoV);
	inline void renderInViewport(const vec4<I32>& rect, boost::function0<void> callback)  {_api.renderInViewport(rect,callback);}
    inline IMPrimitive* createPrimitive() { return _api.createPrimitive(); }

	inline void drawDebugAxis(const bool state)       {_drawDebugAxis = state;}
    inline bool drawDebugAxis()                 const {return _drawDebugAxis;}
	inline void debugDraw() {_api.debugDraw();}
	void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset);
	void drawLines(const vectorImpl<vec3<F32> >& pointsA,
				   const vectorImpl<vec3<F32> >& pointsB,
				   const vectorImpl<vec4<U8> >& colors, 
				   const mat4<F32>& globalOffset,
				   const bool orthoMode = false,
				   const bool disableDepth = false);
    ///Usefull to perform pre-draw operations on the model if it's drawn outside the scenegraph
	void renderInstance(RenderInstance* const instance);
	void renderBuffer(VertexBufferObject* const vbo, Transform* const vboTransform = NULL);
	void renderGUIElement(GUIElement* const guiElement,ShaderProgram* const guiShader);
	///The render callback must update all visual information and populate the "RenderBin"'s!
	///Use the sceneGraph::update callback as default using the macro SCENE_GRAPH_UPDATE(pointer)
	///pointer = a pointer to the sceneGraph instance used for rendering
	void render(boost::function0<void> renderFunction,SceneRenderState* const sceneRenderState);
	///Update light properties internally in the Rendering API
	inline void setLight(Light* const light)           {_api.setLight(light);}
	///Sets the current render state.
	///@param stage Is used to inform the rendering pipeline what we are rendering. Shadows? reflections? etc
	inline void  setRenderStage(RenderStage stage) {_renderStage = stage;}
    ///Checks if the current rendering stage is any of the stages defined in renderStageMask
	///@param renderStageMask Is a bitmask of the stages we whish to check if active
		   bool isCurrentRenderStage(U16 renderStageMask);
           bool getDeferredRendering();

	inline RenderStage  getRenderStage()                 {return _renderStage;}
	inline void         setPrevShaderId(const U32& id)   {_prevShaderId = id;}
	inline U32          getPrevShaderId()                {return _prevShaderId;}
	inline void         setPrevTextureId(const U32& id)  {_prevTextureId = id;}
	inline U32          getPrevTextureId()               {return _prevTextureId;}
	inline Renderer*    getRenderer()                    {assert(_renderer != NULL); return _renderer;}
	       void         setRenderer(Renderer* const renderer);
		   void         closeRenderer();
	///Save a screenshot in TGA format
	inline void Screenshot(char *filename, const vec4<F32>& rect){_api.Screenshot(filename,rect);}
	/// Some Scene Node Types are excluded from certain operations (lights triggers, etc)
	bool excludeFromStateChange(const SceneNodeType& currentType);
	///Creates a new API dependend stateblock based on the received description
	///Calls newRenderStateBlock and also saves the new block in the state block map
	RenderStateBlock* GFXDevice::createStateBlock(const RenderStateBlockDescriptor& descriptor);

	///Sets the current state block to the one passed as a param
	///It is not set immediately, but a call to "updateStates" is required
    RenderStateBlock* setStateBlock(RenderStateBlock* block, bool forceUpdate = false);
	///Same as setStateBlock(RenderStateBlock* block), but uses a blank description
    RenderStateBlock* setStateBlockByDesc(const RenderStateBlockDescriptor &desc);
	///Set previous state block - (deep, I know -Ionut)
    inline RenderStateBlock* setPreviousStateBlock(bool forceUpdate = false) {assert(_previousStateBlock != NULL);return setStateBlock(_previousStateBlock,forceUpdate);}
	///Sets a standard state block
	inline RenderStateBlock* setDefaultStateBlock(bool forceUpdate = false) {assert(_defaultStateBlock != NULL);return setStateBlock(_defaultStateBlock,forceUpdate);}
	///If a new state has been set, update the Graphics pipeline
		   void updateStates(bool force = false);
	///Update the graphics pipeline using the current rendering API with the state block passed
	inline void updateStateInternal(RenderStateBlock* block, bool force = false) {_api.updateStateInternal(block,force);}
	/*//Render State Management */

	///Generate a cubemap from the given position
	///It renders the entire scene graph (with culling) as default
	///use the callback param to override the draw function
	void  generateCubeMap(FrameBufferObject& cubeMap,
						  Camera* const activeCamera,
						  const vec3<F32>& pos,
						  boost::function0<void> callback = 0);

	inline bool loadInContext(const CurrentContext& context, boost::function0<void> callback) {return _api.loadInContext(context, callback);}
	inline void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat)     {_api.getMatrix(mode, mat);}
    inline void getMatrix(const EXTENDED_MATRIX& mode, mat4<F32>& mat)   {_api.getMatrix(mode, mat);}
    inline void getMatrix(const EXTENDED_MATRIX& mode, mat3<F32>& mat)   {_api.getMatrix(mode, mat);}

#if defined( __WIN32__ ) || defined( _WIN32 )
	HWND getHWND() {return _api.getHWND();}
#elif defined( __APPLE_CC__ ) // Apple OS X
	??
#else //Linux
	Display* getDisplay() {return _api.getDisplay();}
	GLXDrawable getDrawSurface() {return _api.getDrawSurface();}
#endif
private:

	GFXDevice();

	inline F32 xfov_to_yfov(F32 xfov, F32 aspect) {
		return DEGREES(2.0f * atan(tan(RADIANS(xfov) * 0.5f) / aspect));
	}

	inline F32 yfov_to_xfov(F32 yfov, F32 aspect) {
		return DEGREES(2.0f * atan(tan(RADIANS(yfov) * 0.5f) * aspect));
	}

	///Returns an API dependend stateblock based on the description
	inline RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor) {
		return _api.newRenderStateBlock(descriptor);
	}

    inline void drawText(const std::string& text, const I32 width, const std::string& fontName, const F32 fontSize) {_api.drawText(text,width,fontName,fontSize);}
	inline void drawText(const std::string& text, const I32 width, const vec2<I32> position, const std::string& fontName, const F32 fontSize) {_api.drawText(text,width,position,fontName,fontSize);}

private:
	RenderAPIWrapper& _api;
	bool _deviceStateDirty;
	RenderStage _renderStage;
    U32  _prevShaderId,  _prevTextureId;
    bool _drawDebugAxis;

protected:
	friend class GL_API;
	friend class DX_API;
	Renderer* _renderer;
	/*State management */
	typedef Unordered_map<I64, RenderStateBlock* > RenderStateMap;
	RenderStateMap _stateBlockMap;
    bool _stateBlockDirty;
	bool _stateBlockByDescription;
	RenderStateBlock* _currentStateBlock;
    RenderStateBlock* _newStateBlock;
	RenderStateBlock* _previousStateBlock;
	RenderStateBlock* _defaultStateBlock;
	///Pointer to current kernel
	Kernel* _kernel;
END_SINGLETON

#define GFX_DEVICE GFXDevice::getInstance()
#define GFX_RENDER_BIN_SIZE RenderPassManager::getInstance().getLastTotalBinSize(0)

inline RenderStateBlock* SET_STATE_BLOCK(RenderStateBlock* const block, bool forceUpdate = false){
    return GFX_DEVICE.setStateBlock(block,forceUpdate);
}

inline RenderStateBlock* SET_DEFAULT_STATE_BLOCK(bool forceUpdate = false){
    return GFX_DEVICE.setDefaultStateBlock(forceUpdate);
}

inline RenderStateBlock* SET_PREVIOUS_STATE_BLOCK(bool forceUpdate = false){
    return GFX_DEVICE.setPreviousStateBlock(forceUpdate);
}
#endif
