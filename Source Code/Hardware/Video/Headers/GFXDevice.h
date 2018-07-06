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
    typedef std::stack<mat4<F32>, vectorImpl<mat4<F32> > > matrixStackW;
    typedef Unordered_map<I64, RenderStateBlock* > RenderStateMap;
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

    ///Hardware specific shader preps (e.g.: OpenGL: init/deinit GLSL-OPT and GLSW)
    inline bool initShaders()   { return _api.initShaders(); }
    inline bool deInitShaders() { return _api.deInitShaders(); }
    inline void beginFrame()    {_api.beginFrame();}
    inline void endFrame()      {_api.endFrame();  }
    inline void flush();

    /// Rendering buffer management
    inline FrameBuffer*        getRenderTarget(RenderTarget target) const  {return _renderTarget[target];}

    inline FrameBuffer*        newFB(bool multisampled = false)              const { return _api.newFB(multisampled); }
    inline VertexBuffer*       newVB()                                       const { return _api.newVB(); }
    inline PixelBuffer*        newPB(const PBType& type = PB_TEXTURE_2D)     const { return _api.newPB(type); }
    inline GenericVertexData*  newGVD(const bool persistentMapped)           const { return _api.newGVD(persistentMapped); }
    inline ShaderBuffer*       newSB(const bool unbound = false)             const { return _api.newSB(unbound); }
    inline Texture*            newTextureArray(const bool flipped = false)   const { return _api.newTextureArray(flipped); }
    inline Texture*            newTexture2D(const bool flipped = false)      const { return _api.newTexture2D(flipped); }
    inline Texture*            newTextureCubemap(const bool flipped = false) const { return _api.newTextureCubemap(flipped);}
    inline ShaderProgram*      newShaderProgram(const bool optimise = false) const { return _api.newShaderProgram(optimise); }
    inline Shader*             newShader(const std::string& name, const  ShaderType& type, const bool optimise = false) const {
        return _api.newShader(name,type,optimise); 
    }

    void enableFog(F32 density, const vec3<F32>& color);
    void toggle2D(bool state);
    inline void toggleRasterization(bool state);
    inline void renderInViewport(const vec4<I32>& rect, const DELEGATE_CBK& callback);

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
    void renderInstanceCmd(RenderInstance* const instance, const GenericDrawCommand& cmd);
    ///The render callback must update all visual information and populate the "RenderBin"'s!
    void render(const DELEGATE_CBK& renderFunction, const SceneRenderState& sceneRenderState);
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
    inline Renderer*    getRenderer()                         {
        DIVIDE_ASSERT(_renderer != nullptr, "GFXDevice error: Renderer requested but not created!"); 
        return _renderer;
    }
    inline void         setRenderer(Renderer* const renderer) {
        DIVIDE_ASSERT(renderer != nullptr, "GFXDevice error: Tried to create an invalid renderer!"); 
        SAFE_UPDATE(_renderer, renderer);
    }
    /*
    /* Clipping plane management. All the clipping planes are handled by shader programs only!
    */
          void updateClipPlanes();
    /// disable a clip plane by index
    inline void GFXDevice::disableClipPlane(ClipPlaneIndex index) { assert(index != ClipPlaneIndex_PLACEHOLDER); _clippingPlanes[index].active(false); _api.updateClipPlanes(); }
    /// enable a clip plane by index
    inline void GFXDevice::enableClipPlane(ClipPlaneIndex index)  { assert(index != ClipPlaneIndex_PLACEHOLDER); _clippingPlanes[index].active(true);  _api.updateClipPlanes(); }
    /// modify a single clip plane by index
    inline void GFXDevice::setClipPlane(ClipPlaneIndex index, const Plane<F32>& p) { assert(index != ClipPlaneIndex_PLACEHOLDER); _clippingPlanes[index] = p; updateClipPlanes(); }
    /// set a new list of clipping planes. The old one is discarded
    inline void GFXDevice::setClipPlanes(const PlaneList& clipPlanes)  {
        if (clipPlanes != _clippingPlanes) {
            _clippingPlanes = clipPlanes;
            updateClipPlanes();
            _api.updateClipPlanes(); 
        }
    }

    /// clear all clipping planes
    inline void GFXDevice::resetClipPlanes() {
        _clippingPlanes.resize(Config::MAX_CLIP_PLANES, Plane<F32>(0,0,0,0));
        updateClipPlanes();
        _api.updateClipPlanes(); 
    }

    /// get the entire list of clipping planes
    inline const PlaneList& getClippingPlanes()         const {return _clippingPlanes;}
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
    void Screenshot(char *filename);
    /// Some Scene Node Types are excluded from certain operations (lights triggers, etc)
    inline bool excludeFromStateChange(const SceneNodeType& currentType) {
        return (_stateExclusionMask & currentType) == currentType ? true : false;
    }
    inline void setStateChangeExclusionMask(I32 stateMask) {_stateExclusionMask = stateMask; }
    ///Creates a new API dependent stateblock based on the received description
    ///Sets the current state block to the one passed as a param
    ///It is not set immediately, but a call to "updateStates" is required
    I64 setStateBlock(I64 stateBlockHash, bool forceUpdate = false);
    /// Return or create a new state block using the given descriptor. DO NOT DELETE THE RETURNED STATE BLOCK! GFXDevice handles that!
    I64 getOrCreateStateBlock(const RenderStateBlockDescriptor& descriptor);
    const RenderStateBlockDescriptor& getStateBlockDescriptor(I64 renderStateBlockHash) const;
    ///Sets a standard state block
    inline I64 setDefaultStateBlock(bool forceUpdate = false)  {return setStateBlock(_defaultStateBlockHash, forceUpdate);}
    ///If a new state has been set, update the Graphics pipeline
           void updateStates();
    /*//Render State Management */

    ///Generate a cubemap from the given position
    ///It renders the entire scene graph (with culling) as default
    ///use the callback param to override the draw function
    void  generateCubeMap(FrameBuffer& cubeMap,
                          const vec3<F32>& pos,
                          const DELEGATE_CBK& callback, 
                          const vec2<F32>& zPlanes,
                          const RenderStage& renderStage = REFLECTION_STAGE);

    /// Matrix management 
           void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat);
           void getMatrix(const EXTENDED_MATRIX& mode, mat3<F32>& mat);
           void getMatrix(const EXTENDED_MATRIX& mode, mat4<F32>& mat);
           void pushWorldMatrix(const mat4<F32>& worldMatrix, const bool isUniformedScaled);
           void popWorldMatrix(); 
           void cleanMatrices();

    inline static ShaderProgram* getActiveProgram()  { return _activeShaderProgram; }

    inline const mat4<F32>& getMatrix(const MATRIX_MODE& mode)      { getMatrix(mode, _mat4Cache); return _mat4Cache; }
    inline const mat4<F32>& getMatrix4(const EXTENDED_MATRIX& mode) { getMatrix(mode, _mat4Cache); return _mat4Cache; }
    inline const mat3<F32>& getMatrix3(const EXTENDED_MATRIX& mode) { getMatrix(mode, _mat3Cache); return _mat3Cache; }

    inline I32 getMaxTextureUnits()  const { return _maxTextureSlots; }
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

    void      restoreViewport();
    vec4<I32> setViewport(const vec4<I32>& viewport, bool force = false);

    bool loadInContext(const CurrentContext& context, const DELEGATE_CBK& callback);

    void ConstructHIZ();
    void DownSampleDepthBuffer(vectorImpl<vec2<F32>> &depthRanges);

    void processVisibleNodes(const vectorImpl<SceneGraphNode* >& visibleNodes);

protected:
    typedef boost::lockfree::spsc_queue<DELEGATE_CBK, boost::lockfree::capacity<15> > LoadQueue;

    friend class Kernel;
    friend class Application;
    inline void setMousePosition(U16 x, U16 y) const {_api.setMousePosition(x,y);}
    inline void changeResolutionInternal(U16 width, U16 height)       { _api.changeResolutionInternal(width, height); }
    inline void changeViewport(const vec4<I32>& newViewport)    const { _api.changeViewport(newViewport); }
    inline void loadInContextInternal()                               { _api.loadInContextInternal(); }

    inline LoadQueue& getLoadQueue() { return _loadQueue; }

protected:
    friend class Camera;

    F32* lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& eyePos);
    ///sets an ortho projection, updating any listeners if needed
    F32* setProjection(const vec4<F32>& rect, const vec2<F32>& planes);
    ///sets a perspective projection, updating any listeners if needed
    F32* setProjection(F32 FoV, F32 aspectRatio, const vec2<F32>& planes) ;
    ///sets the view frustum to either the left or right eye position for anaglyph rendering
    void setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes, F32 aspectRatio, F32 verticalFoV, bool rightFrustum = false);

    void updateViewMatrix(const vec3<F32>& eyePos);
    void updateProjectionMatrix(const vec2<F32>& zPlanes);

private:

    GFXDevice();
    ~GFXDevice();
    void previewDepthBuffer();
    void updateViewportInternal(const vec4<I32>& viewport);
    ///Update the graphics pipeline using the current rendering API with the state block passed
    inline void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock)  const { _api.activateStateBlock(newBlock, oldBlock); }

private:
    Camera*           _cubeCamera;
    Camera*           _2DCamera;
    PostFX&           _postFX;
    ShaderManager&    _shaderManager;
    RenderAPIWrapper& _api;
    RenderStage _renderStage;
    U32  _prevShaderId,  _prevTextureId;
    I32  _stateExclusionMask;
    bool _drawDebugAxis;
    bool _isDepthPrePass;
    bool _viewportUpdate;
    bool _viewportForced;
    LoadQueue _loadQueue;
    boost::thread *_loaderThread;
    static ShaderProgram* _activeShaderProgram;
    
protected:
    friend class GL_API;
    friend class DX_API;
    Renderer* _renderer;
    /* Rendering buffers*/
    FrameBuffer* _renderTarget[RenderTarget_PLACEHOLDER];
    FrameBuffer* _depthRanges;
    /*State management */
    RenderStateMap _stateBlockMap;
    bool _stateBlockDirty;
    bool _stateBlockByDescription;
    I64 _currentStateBlockHash;
    I64 _newStateBlockHash;
    I64 _defaultStateBlockHash;
    I64 _defaultStateNoDepthHash; //<The default render state buth with depth testing disabled
    I64 _state2DRenderingHash;    //<Special render state for 2D rendering
    I64 _stateDepthOnlyRenderingHash;
    matrixStackW  _worldMatrices;
    mat4<F32> _textureMatrix;
    mat4<F32> _viewMatrix;
    mat4<F32> _projectionMatrix;
    mat4<F32> _viewProjectionMatrix;
    mat4<F32> _WVCachedMatrix;
    mat4<F32> _VPCachedMatrix;
    mat4<F32> _WVPCachedMatrix;
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
    bool      _enablePostProcessing;
    bool      _enableAnaglyph;
    bool      _enableHDR;
    bool      _2DRendering;
    bool      _rasterizationEnabled;
    ///shader used to preview the depth buffer
    ShaderProgram* _previewDepthMapShader;
    static ShaderProgram* _HIZConstructProgram;
    static ShaderProgram* _depthRangesConstructProgram;
    bool    _previewDepthBuffer;
    ///getMatrix cache
    mat4<F32> _mat4Cache;
    mat3<F32> _mat3Cache;
    ///Default camera's cached zPlanes
    vec2<F32> _cachedSceneZPlanes;
    /// AA system
    U8        _MSAASamples;
    U8        _FXAASamples;
    // Texture slots
    I32       _maxTextureSlots;
    /// Quality settings
    RenderDetailLevel _generalDetailLevel;
    RenderDetailLevel _shadowDetailLevel;

    vectorImpl<std::pair<U32, DELEGATE_CBK> > _2dRenderQueue;

    ///Current viewport stack
    typedef std::stack<vec4<I32>, vectorImpl<vec4<I32> > > viewportStack;
    viewportStack _viewport;

    vectorImpl<SceneGraphNode::NodeShaderData> _nodeData;

    ShaderBuffer*  _matricesBuffer;
    ShaderBuffer*  _nodeDataBuffer;
END_SINGLETON

#include "GFXDevice-Inl.h"

#endif
