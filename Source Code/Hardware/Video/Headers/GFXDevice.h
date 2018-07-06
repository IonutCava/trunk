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

#ifndef _HARDWARE_VIDEO_GFX_DEVICE_H_
#define _HARDWARE_VIDEO_GFX_DEVICE_H_

#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
#include "Hardware/Video/Direct3D/Headers/DXWrapper.h"
#include "Managers/Headers/RenderPassManager.h" ///<For GFX_RENDER_BIN_SIZE
#include "Rendering/Headers/Renderer.h"

#ifdef FORCE_NV_OPTIMUS_HIGHPERFORMANCE
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

enum RenderStage;
enum SceneNodeType;

class Light;
class Camera;
class Object3D;
class Framerate;
class SceneRenderState;
///Rough around the edges Adapter pattern
DEFINE_SINGLETON_EXT1(GFXDevice,RenderAPIWrapper)
friend class Frustum; ///< For matrix recovery operations

public:

    void setApi(const RenderAPI& api);

    inline RenderAPI        getApi()        {return _api.getId(); }
    inline RenderAPIVersion getApiVersion() {return _api.getVersionId();}
    inline GPUVendor        getGPUVendor()  {return _api.getGPUVendor();}

    inline I8   initHardware(const vec2<U16>& resolution, I32 argc, char **argv){return _api.initHardware(resolution,argc,argv);}

    inline void      registerKernel(Kernel* const kernel)           {_kernel = kernel;}
    inline void      initDevice(U32 targetFrameRate)                {_api.initDevice(targetFrameRate);}
    inline void      changeResolution(U16 w, U16 h)                 {_api.changeResolution(w,h);}
    inline void      setWindowPos(U16 w, U16 h)               const {_api.setWindowPos(w,h);}
    inline vec3<F32> unproject(const vec3<F32>& windowCoord)  const {return _api.unproject(windowCoord); }
    inline void      exitRenderLoop(bool killCommand = false)       {_api.exitRenderLoop(killCommand);}
           void      closeRenderingApi();

    inline void beginFrame() {_api.beginFrame();}
    inline void endFrame()   {_api.endFrame();  }
    inline void idle()       {_api.idle();}
    inline void flush()      {_api.flush();}

    inline Shader*             newShader(const std::string& name,
                                         const  ShaderType& type,
                                         const bool optimise = false)                 {return _api.newShader(name,type,optimise); }
    inline FrameBufferObject*  newFBO(const FBOType& type = FBO_2D_COLOR)             {return _api.newFBO(type); }
    inline VertexBufferObject* newVBO(const PrimitiveType& type = TRIANGLES)          {return _api.newVBO(type); }
    inline PixelBufferObject*  newPBO(const PBOType& type = PBO_TEXTURE_2D)           {return _api.newPBO(type); }
    inline Texture2D*          newTexture2D(const bool flipped = false)               {return _api.newTexture2D(flipped);}
    inline TextureCubemap*     newTextureCubemap(const bool flipped = false)          {return _api.newTextureCubemap(flipped);}
    inline ShaderProgram*      newShaderProgram(const bool optimise = false)          {return _api.newShaderProgram(optimise); }

    ///Hardware specific shader preps (e.g.: OpenGL: init/deinit GLSL-OPT and GLSW)
    inline bool            initShaders()                                        {return _api.initShaders();}
    inline bool            deInitShaders()                                      {return _api.deInitShaders();}

    void enableFog(FogMode mode, F32 density, const vec3<F32>& color, F32 startDist, F32 endDist);

    inline void toggle2D(bool _2D)  {_api.toggle2D(_2D);}
    inline void lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& viewDirection)	{ _api.lookAt(viewMatrix, viewDirection); }
    inline void lookAt(const vec3<F32>& eye, const vec3<F32>& target, const vec3<F32>& up = WORLD_Y_AXIS) {_api.lookAt(eye,target,up);}
    ///Usually, after locking and releasing our matrices we want to revert to the View matrix to render geometry
    inline void lockMatrices(const MATRIX_MODE& setCurrentMatrix = VIEW_MATRIX, bool lockView = true, bool lockProjection = true)          {_api.lockMatrices(setCurrentMatrix,lockView,lockProjection);}
    inline void releaseMatrices(const MATRIX_MODE& setCurrentMatrix = VIEW_MATRIX, bool releaseView = true, bool releaseProjection = true) {_api.releaseMatrices(setCurrentMatrix,releaseView,releaseProjection);}
    ///sets an ortho projection, updating any listeners if needed
    inline void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes){_api.setOrthoProjection(rect,planes);}
    ///sets a perspective projection, updating any listeners if needed
    inline void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes) { _api.setPerspectiveProjection(FoV,aspectRatio,planes);}
    ///sets the view frustum to either the left or right eye position for anaglyph rendering
    inline void setAnaglyphFrustum(F32 camIOD, bool rightFrustum = false)  {_api.setAnaglyphFrustum(camIOD,rightFrustum);}
            ///sets a new horizontal FoV
            void setHorizontalFoV(I32 newFoV);
    inline void renderInViewport(const vec4<U32>& rect, boost::function0<void> callback)  {_api.renderInViewport(rect,callback);}
    //returns an immediate mode emulation buffer that can be used to construct geometry in a vertex by vertex manner.
    //allowPrimitiveRecycle = do not reause old primitives and do not delete it after x-frames. (Don't use the primitive zombie feature)
    inline IMPrimitive* createPrimitive(bool allowPrimitiveRecycle = true) { return _api.createPrimitive(allowPrimitiveRecycle); }

    inline void drawDebugAxis(const bool state)       {_drawDebugAxis = state;}
    inline bool drawDebugAxis()                 const {return _drawDebugAxis;}
    inline void debugDraw()                           {_api.debugDraw();}

    inline void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset) {_api.drawBox3D(min,max,globalOffset);}
    inline void drawLines(const vectorImpl<vec3<F32> >& pointsA,
                          const vectorImpl<vec3<F32> >& pointsB,
                          const vectorImpl<vec4<U8> >& colors,
                          const mat4<F32>& globalOffset,
                          const bool orthoMode = false,
                          const bool disableDepth = false) {_api.drawLines(pointsA,pointsB,colors,globalOffset,orthoMode,disableDepth);}
    ///Usefull to perform pre-draw operations on the model if it's drawn outside the scenegraph
    void renderInstance(RenderInstance* const instance);
    void renderBuffer(VertexBufferObject* const vbo, Transform* const vboTransform = NULL);
    void renderGUIElement(GUIElement* const guiElement,ShaderProgram* const guiShader);
    ///The render callback must update all visual information and populate the "RenderBin"'s!
    ///Use the sceneGraph::update callback as default using the macro SCENE_GRAPH_UPDATE(pointer)
    ///pointer = a pointer to the sceneGraph instance used for rendering
    void render(boost::function0<void> renderFunction, const SceneRenderState& sceneRenderState);
    ///Update light properties internally in the Rendering API
    inline void setLight(Light* const light)           {_api.setLight(light);}
    ///Sets the current render stage.
    ///@param stage Is used to inform the rendering pipeline what we are rendering. Shadows? reflections? etc
    inline void  setRenderStage(RenderStage stage) {_prevRenderStage = _renderStage; _renderStage = stage;}
    ///Restores the render stage to the previous one calling it multiple times will ping-pong between stages
    inline void  setPreviousRenderStage()          {setRenderStage(_prevRenderStage);}
    ///Checks if the current rendering stage is any of the stages defined in renderStageMask
    ///@param renderStageMask Is a bitmask of the stages we whish to check if active
           bool         isCurrentRenderStage(U16 renderStageMask);
    inline RenderStage  getRenderStage()                 {return _renderStage;}
    inline void         setPrevShaderId(const U32& id)   {_prevShaderId = id;}
    inline U32          getPrevShaderId()                {return _prevShaderId;}
    inline void         setPrevTextureId(const U32& id)  {_prevTextureId = id;}
    inline U32          getPrevTextureId()               {return _prevTextureId;}
    inline Renderer*    getRenderer()                    {assert(_renderer != NULL); return _renderer;}
           void         setRenderer(Renderer* const renderer);
           void         closeRenderer();

    /*
    /* Clipping plane management. All the clipping planes are handled by shader programs only!
    */
    ///enable-disable HW clipping
    inline void updateClipPlanes() {_api.updateClipPlanes(); }
    /// add a new clipping plane. This will be limited by the actual shaders (how many planes they use)
    /// this function returns the newly added clip plane's index in the vector
    inline I32 addClipPlane(const Plane<F32>& p);
    /// add a new clipping plane defined by it's equation's coefficients
    inline I32 addClipPlane(F32 A, F32 B, F32 C, F32 D);
    /// remove a clip plane by index
    inline bool removeClipPlane(U32 index);
    /// disable a clip plane by index
    inline bool disableClipPlane(U32 index);
    /// enable a clip plane by index
    inline bool enableClipPlane(U32 index);
    /// modify a single clip plane by index
    inline void setClipPlane(U32 index, const Plane<F32>& p);
    /// set a new list of clipping planes. The old one is discarded
    inline void setClipPlanes(const PlaneList& clipPlanes);
    /// clear all clipping planes
    inline void resetClipPlanes();
    /// have the clipping planes changed?
    inline bool clippingPlanesDirty()           const {return _clippingPlanesDirty;}
    /// get the entire list of clipping planes
    inline const PlaneList& getClippingPlanes() const {return _clippingPlanes;}
    ///Save a screenshot in TGA format
    inline void Screenshot(char *filename, const vec4<F32>& rect){_api.Screenshot(filename,rect);}
    /// Some Scene Node Types are excluded from certain operations (lights triggers, etc)
    bool excludeFromStateChange(const SceneNodeType& currentType);
    ///Creates a new API dependend stateblock based on the received description
    ///Calls newRenderStateBlock and also saves the new block in the state block map
    RenderStateBlock* createStateBlock(const RenderStateBlockDescriptor& descriptor);
    ///Sets the current state block to the one passed as a param
    ///It is not set immediately, but a call to "updateStates" is required
    RenderStateBlock* setStateBlock(RenderStateBlock* block, bool forceUpdate = false);
    ///Same as setStateBlock(RenderStateBlock* block), but uses a blank description
    RenderStateBlock* setStateBlockByDesc(const RenderStateBlockDescriptor &desc);
    ///Set previous state block - (deep, I know -Ionut)
    inline RenderStateBlock* setPreviousStateBlock(bool forceUpdate = false) {assert(_previousStateBlock != NULL);return setStateBlock(_previousStateBlock,forceUpdate);}
    ///Sets a standard state block
    inline RenderStateBlock* setDefaultStateBlock(bool forceUpdate = false) {assert(_defaultStateBlock != NULL);return setStateBlock(_defaultStateBlock,forceUpdate);}
    ///Update the graphics pipeline using the current rendering API with the state block passed
    inline void updateStateInternal(RenderStateBlock* block, bool force = false) {_api.updateStateInternal(block,force);}
    ///If a new state has been set, update the Graphics pipeline
           void updateStates(bool force = false);
    /*//Render State Management */

    ///Generate a cubemap from the given position
    ///It renders the entire scene graph (with culling) as default
    ///use the callback param to override the draw function
    void  generateCubeMap(FrameBufferObject& cubeMap,
                          const vec3<F32>& pos,
                          boost::function0<void> callback = 0,
                          const RenderStage& renderStage = ENVIRONMENT_MAPPING_STAGE);

    inline bool loadInContext(const CurrentContext& context, boost::function0<void> callback) {return _api.loadInContext(context, callback);}
    inline void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat)       {_api.getMatrix(mode, mat);}
    inline void getMatrix(const EXTENDED_MATRIX& mode, mat4<F32>& mat)   {_api.getMatrix(mode, mat);}
    inline void getMatrix(const EXTENDED_MATRIX& mode, mat3<F32>& mat)   {_api.getMatrix(mode, mat);}

#if defined(OS_WINDOWS)
    HWND getHWND() {return _api.getHWND();}
#elif defined(OS_APPLE)
    ??
#else //Linux
    Display* getDisplay() {return _api.getDisplay();}
    GLXDrawable getDrawSurface() {return _api.getDrawSurface();}
#endif

protected:
    friend class Kernel;
    friend class Application;
    inline void setMousePosition(D32 x, D32 y) const {_api.setMousePosition(x,y);}

private:

    GFXDevice();

    ///Returns an API dependend stateblock based on the description
    inline RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor) { return _api.newRenderStateBlock(descriptor); }

    inline void drawText(const std::string& text, const I32 width, const std::string& fontName, const F32 fontSize) {_api.drawText(text,width,fontName,fontSize);}
    inline void drawText(const std::string& text, const I32 width, const vec2<I32> position, const std::string& fontName, const F32 fontSize) {_api.drawText(text,width,position,fontName,fontSize);}

private:
    RenderAPIWrapper& _api;
    bool _deviceStateDirty;
    RenderStage _renderStage, _prevRenderStage;
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
    PlaneList _clippingPlanes;
    bool      _clippingPlanesDirty;

END_SINGLETON

#include "GFXDevice-Inl.h"

#endif
