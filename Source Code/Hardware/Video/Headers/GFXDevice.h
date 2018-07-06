/*
   Copyright (c) 2014 DIVIDE-Studio
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

#include "RenderInstance.h"
#include "Hardware/Video/Textures/Headers/Texture.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
#include "Hardware/Video/Direct3D/Headers/DXWrapper.h"
#include "Managers/Headers/RenderPassManager.h" ///<For GFX_RENDER_BIN_SIZE
#include "Rendering/Headers/Renderer.h"

enum RenderStage;
enum SceneNodeType;

class Light;
class Camera;
class PostFX;
class Quad3D;
class Object3D;
class ApplicationTimer;
class SceneRenderState;

class GFXDevice;

///Rough around the edges Adapter pattern
DEFINE_SINGLETON_EXT1(GFXDevice,RenderAPIWrapper)
friend class Frustum; ///< For matrix recovery operations
typedef std::stack<mat4<F32>, vectorImpl<mat4<F32> > > matrixStack;

public:
    enum RenderTarget {
        RENDER_TARGET_SCREEN = 0,
        RENDER_TARGET_ANAGLYPH = 1,
        RENDER_TARGET_DEPTH = 2,
        RenderTarget_PLACEHOLDER = 3
    };

    enum SortReservedValues {
        SORT_DEPTH_STAGE = -222,
        SORT_NO_VALUE = -333
    };

    void setApi(const RenderAPI& api);

    inline RenderAPI        getApi()        {return _api.getId(); }
    inline GPUVendor        getGPUVendor()  {return _api.getGPUVendor();}

    I8 initHardware(const vec2<U16>& resolution, I32 argc, char **argv);
    void idle();

    inline void      registerKernel(Kernel* const kernel)           {_kernel = kernel;}
    inline void      initDevice(U32 targetFrameRate)                {_api.initDevice(targetFrameRate);}
    inline void      setWindowPos(U16 w, U16 h)               const {_api.setWindowPos(w,h);}
    inline void      exitRenderLoop(bool killCommand = false)       {_api.exitRenderLoop(killCommand);}
           void      closeRenderingApi();
           void      changeResolution(U16 w, U16 h);
    inline void      changeResolution(const vec2<U16>& resolution) {changeResolution(resolution.width, resolution.height);}

    inline void beginFrame() {_api.beginFrame();}
    inline void endFrame()   {_api.endFrame();  }
           void flush();

    /// Rendering buffer management
    inline FrameBuffer*        getRenderTarget(RenderTarget target) const  {return _renderTarget[target];}

    inline FrameBuffer*        newFB(bool multisampled = false)              const { return _api.newFB(multisampled); }
    inline VertexBuffer*       newVB(const PrimitiveType& type = TRIANGLES)  const { return _api.newVB(type); }
    inline PixelBuffer*        newPB(const PBType& type = PB_TEXTURE_2D)     const { return _api.newPB(type); }
    inline GenericVertexData*  newGVD()                                      const { return _api.newGVD(); }
    inline Texture*            newTextureArray(const bool flipped = false)   const { return _api.newTextureArray(flipped); }
    inline Texture*            newTexture2D(const bool flipped = false)      const { return _api.newTexture2D(flipped); }
    inline Texture*            newTextureCubemap(const bool flipped = false) const { return _api.newTextureCubemap(flipped);}
    inline ShaderProgram*      newShaderProgram(const bool optimise = false) const { return _api.newShaderProgram(optimise); }
    inline Shader*             newShader(const std::string& name, const  ShaderType& type, const bool optimise = false) const {
        return _api.newShader(name,type,optimise); 
    }
    ///Hardware specific shader preps (e.g.: OpenGL: init/deinit GLSL-OPT and GLSW)
    inline bool            initShaders()    {return _api.initShaders();}
    inline bool            deInitShaders()  {return _api.deInitShaders();}

    void enableFog(F32 density, const vec3<F32>& color);
    void toggle2D(bool state);
    inline void toggleRasterization(bool state);
    inline void renderInViewport(const vec4<I32>& rect, const DELEGATE_CBK& callback)  {_api.renderInViewport(rect,callback);}
    //returns an immediate mode emulation buffer that can be used to construct geometry in a vertex by vertex manner.
    //allowPrimitiveRecycle = do not reause old primitives and do not delete it after x-frames. (Don't use the primitive zombie feature)
    inline IMPrimitive* createPrimitive(bool allowPrimitiveRecycle = true) { return _api.createPrimitive(allowPrimitiveRecycle); }

    inline void setInterpolation(const D32 interpolation)       {_interpolationFactor = interpolation; }
    inline D32  getInterpolation()                        const {return _interpolationFactor;} 
    inline void drawDebugAxis(const bool state)                 {_drawDebugAxis = state;}
    inline bool drawDebugAxis()                           const {return _drawDebugAxis;}

    inline void debugDraw(const SceneRenderState& sceneRenderState) { _api.debugDraw(sceneRenderState); }
    inline void drawText(const TextLabel& textLabel, const vec2<I32>& position) { _api.drawText(textLabel, position); }
    inline void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset) {_api.drawBox3D(min,max,globalOffset);}
    inline void drawLines(const vectorImpl<vec3<F32> >& pointsA,
                          const vectorImpl<vec3<F32> >& pointsB,
                          const vectorImpl<vec4<U8> >& colors,
                          const mat4<F32>& globalOffset,
                          const bool orthoMode = false,
                          const bool disableDepth = false) {_api.drawLines(pointsA,pointsB,colors,globalOffset,orthoMode,disableDepth);}
           void drawPoints(U32 numPoints);
    ///Useful to perform pre-draw operations on the model if it's drawn outside the scenegraph
    void renderInstance(RenderInstance* const instance);
    ///The render callback must update all visual information and populate the "RenderBin"'s!
    void render(const DELEGATE_CBK& renderFunction, const SceneRenderState& sceneRenderState);
    ///Update light properties internally in the Rendering API
    inline void setLight(Light* const light, bool shadowPass = false)           { _api.setLight(light, shadowPass); }
    ///Sets the current render stage.
    ///@param stage Is used to inform the rendering pipeline what we are rendering. Shadows? reflections? etc
    inline RenderStage setRenderStage(RenderStage stage) { RenderStage prevRenderStage = _renderStage; _renderStage = stage; return prevRenderStage; }
    ///Checks if the current rendering stage is any of the stages defined in renderStageMask
    ///@param renderStageMask Is a bitmask of the stages we whish to check if active
    inline bool isCurrentRenderStage(U8 renderStageMask);

    inline const vec4<I32>& getCurrentViewport()  const  {return _viewport.top(); }

    inline RenderStage  getRenderStage()                 {return _renderStage;}
    inline void         setPrevShaderId(const U32& id)   {_prevShaderId = id;}
    inline U32          getPrevShaderId()                {return _prevShaderId;}
    inline void         setPrevTextureId(const U32& id)  {_prevTextureId = id;}
    inline U32          getPrevTextureId()               {return _prevTextureId;}
           void         closeRenderer();
    inline Renderer*    getRenderer()                         {assert(_renderer != nullptr); return _renderer;}
    inline void         setRenderer(Renderer* const renderer) {assert(renderer != nullptr);  SAFE_UPDATE(_renderer, renderer);}
    /*
    /* Clipping plane management. All the clipping planes are handled by shader programs only!
    */
          void updateClipPlanes();
    /// add a new clipping plane. This will be limited by the actual shaders (how many planes they use)
    /// this function returns the newly added clip plane's index in the vector
    inline I32 addClipPlane(Plane<F32>& p, ClipPlaneIndex clipIndex);
    /// add a new clipping plane defined by it's equation's coefficients
    inline I32 addClipPlane(F32 A, F32 B, F32 C, F32 D, ClipPlaneIndex clipIndex);
    /// change the clip plane index of the current plane
    inline bool changeClipIndex(U32 index, ClipPlaneIndex clipIndex);
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
    /// Post Processing state
    inline bool postProcessingEnabled()                 const {return _enablePostProcessing;}
           void postProcessingEnabled(const bool state);
    /// Anaglyph rendering state
    inline bool anaglyphEnabled()                 const {return _enableAnaglyph;}
    inline void anaglyphEnabled(const bool state)       {_enableAnaglyph = state;}
    /// High Dynamic Range rendering
    inline bool hdrEnabled()                 const {return _enableHDR;}
    inline void hdrEnabled(const bool state)       {_enableHDR = state;}
    /// Depth pre-pass
    inline bool isDepthPrePass()                 const { return _isDepthPrePass; }
    inline void isDepthPrePass(const bool state)       { _isDepthPrePass = state;}
    /// 2D rendering enabled
    inline bool is2DRendering()                  const { return _2DRendering; }
    ///Save a screenshot in TGA format
    inline void Screenshot(char *filename, const vec4<F32>& rect){_api.Screenshot(filename,rect);}
    /// Some Scene Node Types are excluded from certain operations (lights triggers, etc)
    inline bool excludeFromStateChange(const SceneNodeType& currentType) {
        return (_stateExclusionMask & currentType) == currentType ? true : false;
    }
    inline void setStateChangeExclusionMask(I32 stateMask) {_stateExclusionMask = stateMask; }
    ///Creates a new API dependent stateblock based on the received description
    ///Sets the current state block to the one passed as a param
    ///It is not set immediately, but a call to "updateStates" is required
    RenderStateBlock* setStateBlock(const RenderStateBlock& block, bool forceUpdate = false);
    /// Return or create a new state block using the given descriptor. DO NOT DELETE THE RETURNED STATE BLOCK! GFXDevice handles that!
    RenderStateBlock* getOrCreateStateBlock(RenderStateBlockDescriptor& descriptor);
    ///Sets a standard state block
    inline RenderStateBlock* setDefaultStateBlock(bool forceUpdate = false)  {return setStateBlock(*_defaultStateBlock, forceUpdate);}
    ///Update the graphics pipeline using the current rendering API with the state block passed
    inline void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock) { _api.activateStateBlock(newBlock, oldBlock); }
    ///If a new state has been set, update the Graphics pipeline
           void updateStates(bool force = false);
    /*//Render State Management */

    ///Generate a cubemap from the given position
    ///It renders the entire scene graph (with culling) as default
    ///use the callback param to override the draw function
    void  generateCubeMap(FrameBuffer& cubeMap,
                          const vec3<F32>& pos,
                          const DELEGATE_CBK& callback, 
                          const vec2<F32>& zPlanes,
                          const RenderStage& renderStage = REFLECTION_STAGE);

    inline bool loadInContext(const CurrentContext& context, const DELEGATE_CBK& callback) {return _api.loadInContext(context, callback);}

    /// Matrix management 
    inline void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat)       {_api.getMatrix(mode, mat);}
           void getMatrix(const EXTENDED_MATRIX& mode, mat3<F32>& mat);
           void getMatrix(const EXTENDED_MATRIX& mode, mat4<F32>& mat);
           void pushWorldMatrix(const mat4<F32>& worldMatrix, const bool isUniformedScaled);
           void popWorldMatrix(); 
           void cleanMatrices();

    inline static ShaderProgram* getActiveProgram()  { return _activeShaderProgram; }

    inline const mat4<F32>& getMatrix(const MATRIX_MODE& mode)      { getMatrix(mode, _mat4Cache); return _mat4Cache; }
    inline const mat4<F32>& getMatrix4(const EXTENDED_MATRIX& mode) { getMatrix(mode, _mat4Cache); return _mat4Cache; }
    inline const mat3<F32>& getMatrix3(const EXTENDED_MATRIX& mode) { getMatrix(mode, _mat3Cache); return _mat3Cache; }

    inline void setViewDirty(const bool state)        { _VDirty = state; }
    inline void setProjectionDirty(const bool state)  {_PDirty = state;}

    inline U64 getFrameDurationGPU() const { return _api.getFrameDurationGPU(); }
    inline I32 getDrawCallCount()    const { return _api.getDrawCallCount(); }
    inline U32 getFrameCount()       const { return _api.getFrameCount(); }
    inline void togglePreviewDepthBuffer() { _previewDepthBuffer = !_previewDepthBuffer; }

    inline bool MSAAEnabled() const { return _MSAASamples > 0; }
    inline U8   MSAASamples() const { return _MSAASamples; }
    inline bool FXAAEnabled() const { return _FXAASamples > 0; }
    inline U8   FXAASamples() const { return _FXAASamples; }

    inline void initAA(U8 fxaaSamples, U8 msaaSamples){
        _FXAASamples = fxaaSamples;
        _MSAASamples = msaaSamples;
    }

    RenderDetailLevel generalDetailLevel()                             const { return _generalDetailLevel; }
    void              generalDetailLevel(RenderDetailLevel detailLevel)      { _generalDetailLevel = detailLevel; }
    RenderDetailLevel shadowDetailLevel()                              const { return _shadowDetailLevel; }
    void              shadowDetailLevel(RenderDetailLevel detailLevel)       { _shadowDetailLevel = detailLevel; }

    inline void add2DRenderFunction(const DELEGATE_CBK& callback, U32 callOrder);

protected:
    friend class Kernel;
    friend class Application;
    inline void setMousePosition(U16 x, U16 y) const {_api.setMousePosition(x,y);}
    inline void changeResolutionInternal(U16 width, U16 height) { _api.changeResolutionInternal(width, height); }

protected:
    friend class Camera;

    inline F32* lookAt(const mat4<F32>& viewMatrix)	{ return _api.lookAt(viewMatrix); }
    ///sets an ortho projection, updating any listeners if needed
    inline F32* setProjection(const vec4<F32>& rect, const vec2<F32>& planes)    { return _api.setProjection(rect, planes); }
    ///sets a perspective projection, updating any listeners if needed
    inline F32* setProjection(F32 FoV, F32 aspectRatio, const vec2<F32>& planes) { return _api.setProjection(FoV, aspectRatio, planes); }
    ///sets the view frustum to either the left or right eye position for anaglyph rendering
    inline void setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes, F32 aspectRatio, F32 verticalFoV, bool rightFrustum = false)  { _api.setAnaglyphFrustum(camIOD, zPlanes, aspectRatio, verticalFoV, rightFrustum); }

private:

    GFXDevice();
    ~GFXDevice();
           void previewDepthBuffer();

private:
    Camera*           _cubeCamera;
    Camera*           _2DCamera;
    PostFX&           _postFX;
    ShaderManager&    _shaderManager;
    RenderAPIWrapper& _api;
    bool _deviceStateDirty;
    RenderStage _renderStage;
    U32  _prevShaderId,  _prevTextureId;
    I32  _stateExclusionMask;
    bool _drawDebugAxis;
    bool _isDepthPrePass;

    static ShaderProgram* _activeShaderProgram;

protected:
    friend class GL_API;
    friend class DX_API;
    Renderer* _renderer;
    /* Rendering buffers*/
    FrameBuffer* _renderTarget[RenderTarget_PLACEHOLDER];
    /*State management */
    typedef Unordered_map<I64, RenderStateBlock* > RenderStateMap;
    RenderStateMap _stateBlockMap;
    bool _stateBlockDirty;
    bool _stateBlockByDescription;
    RenderStateBlock* _currentStateBlock;
    RenderStateBlock* _newStateBlock;
    RenderStateBlock* _defaultStateBlock;
    RenderStateBlock* _defaultStateNoDepth; //<The default render state buth with depth testing disabled
    RenderStateBlock* _state2DRendering;    //<Special render state for 2D rendering
    matrixStack       _worldMatrices;
    mat4<F32> _WVCachedMatrix;
    mat4<F32> _VPCachedMatrix;
    mat4<F32> _WVPCachedMatrix;
    mat4<F32> _viewCacheMatrix;
    mat4<F32> _projectionCacheMatrix;
    //The interpolation factor between the current and the last frame
    D32       _interpolationFactor;
    bool      _isUniformedScaled;
    bool      _WDirty;
    ///If the _view changed, this will change to true
    bool      _VDirty;
    ///If _projection changed, this will change to true
    bool      _PDirty;
    ///Pointer to current kernel
    Kernel*   _kernel;
    PlaneList _clippingPlanes;
    bool      _clippingPlanesDirty;
    bool      _enablePostProcessing;
    bool      _enableAnaglyph;
    bool      _enableHDR;
    bool      _2DRendering;
    bool      _rasterizationEnabled;
    ///shader used to preview the depth buffer
    ShaderProgram* _previewDepthMapShader;
    bool    _previewDepthBuffer;
    ///getMatrix cache
    mat4<F32> _mat4Cache;
    mat3<F32> _mat3Cache;
    /// AA system
    U8        _MSAASamples;
    U8        _FXAASamples;
    /// Quality settings
    RenderDetailLevel _generalDetailLevel;
    RenderDetailLevel _shadowDetailLevel;

    vectorImpl<std::pair<U32, DELEGATE_CBK> > _2dRenderQueue;

    ///Current viewport stack
    typedef std::stack<vec4<I32>, vectorImpl<vec4<I32> > > viewportStack;
    viewportStack _viewport;

END_SINGLETON

#include "GFXDevice-Inl.h"

#endif
