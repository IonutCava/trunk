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

#include "Hardware/Video/Textures/Headers/Texture.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
#include "Hardware/Video/Direct3D/Headers/DXWrapper.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Graphs/Headers/SceneGraphNode.h"

#include <boost/lockfree/spsc_queue.hpp>
#include <stack>

namespace Divide {

enum RenderStage;
enum SceneNodeType;

class Light;
class Camera;
class PostFX;
class Quad3D;
class Object3D;
class Renderer;
class ApplicationTimer;
class SceneRenderState;

/// Rough around the edges Adapter pattern abstracting the actual rendering API and access to the GPU
DEFINE_SINGLETON_EXT1(GFXDevice,RenderAPIWrapper)
    typedef Unordered_map<size_t, RenderStateBlock* > RenderStateMap;
    typedef std::stack<vec4<I32>, vectorImpl<vec4<I32> > > ViewportStack;
    typedef boost::lockfree::spsc_queue<DELEGATE_CBK, boost::lockfree::capacity<15> > LoadQueue;

public:
    struct NodeData{
        mat4<F32> _matrix[4];

        NodeData()
        {
            _matrix[0].identity();
            _matrix[1].identity();
            _matrix[2].zero();
            _matrix[3].zero();
        }
    };

    struct GPUBlock {
        mat4<F32> _ProjectionMatrix;
        mat4<F32> _ViewMatrix;
        mat4<F32> _ViewProjectionMatrix;
        vec4<F32> _cameraPosition;
        vec4<F32> _ViewPort;
        vec4<F32> _ZPlanesCombined; //xy - current, zw - main scene 
        vec4<F32> _clipPlanes[Config::MAX_CLIP_PLANES];
    };
    
    struct GPUVideoMode {
        // width x height
        vec2<U16> _resolution; 
        // R,G,B
        vec3<U8>  _bitDepth;
        // Max supported
        U8        _refreshRate;
 
        bool operator==(const GPUVideoMode &other) const { 
            return _resolution == other._resolution && 
                   _bitDepth == other._bitDepth && 
                   _refreshRate == other._refreshRate; 
        }
    };

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

    inline void setApi(const RenderAPI& api);

    inline RenderAPI getApi()        {return _api.getId(); }
    inline GPUVendor getGPUVendor()  {return _api.getGPUVendor();}

    ErrorCode initRenderingApi(const vec2<U16>& resolution, I32 argc, char **argv);
    void      closeRenderingApi();

    void idle();

    inline void      registerKernel(Kernel* const kernel)       {_kernel = kernel;}
    inline void      setWindowPos(U16 w, U16 h)           const {_api.setWindowPos(w,h);}
           void      changeResolution(U16 w, U16 h);
    inline void      changeResolution(const vec2<U16>& resolution) {changeResolution(resolution.width, resolution.height);}

    ///Hardware specific shader preps (e.g.: OpenGL: init/deinit GLSL-OPT and GLSW)
    inline bool initShaders()   { return _api.initShaders(); }
    inline bool deInitShaders() { return _api.deInitShaders(); }
           void beginFrame();
           void endFrame();
    
    inline Framebuffer*        getRenderTarget(RenderTarget target)          const { return _renderTarget[target]; }
    inline IMPrimitive*        newIMP()                                      const { return _api.newIMP(); }
    inline Framebuffer*        newFB(bool multisampled = false)              const { return _api.newFB(multisampled); }
    inline VertexBuffer*       newVB()                                       const { return _api.newVB(); }
    inline PixelBuffer*        newPB(const PBType& type = PB_TEXTURE_2D)     const { return _api.newPB(type); }
    inline GenericVertexData*  newGVD(const bool persistentMapped)           const { return _api.newGVD(persistentMapped); }
    inline Texture*            newTextureArray(const bool flipped = false)   const { return _api.newTextureArray(flipped); }
    inline Texture*            newTexture2D(const bool flipped = false)      const { return _api.newTexture2D(flipped); }
    inline Texture*            newTextureCubemap(const bool flipped = false) const { return _api.newTextureCubemap(flipped);}
    inline ShaderProgram*      newShaderProgram(const bool optimise = false) const { return _api.newShaderProgram(optimise); }
    inline Shader*             newShader(const std::string& name, const  ShaderType& type, const bool optimise = false) const {
        return _api.newShader(name,type,optimise); 
    }
    inline ShaderBuffer* newSB(const bool unbound = false, const bool persistentMapped = true) const {
        return _api.newSB(unbound, persistentMapped); 
    }

    void enableFog(F32 density, const vec3<F32>& color);
    void toggle2D(bool state);

    inline void toggleRasterization(bool state);
    inline void setLineWidth(F32 width) { _previousLineWidth = _currentLineWidth; _currentLineWidth = width; _api.setLineWidth(width); }
    inline void restoreLineWidth()      { setLineWidth(_previousLineWidth); }
    inline void renderInViewport(const vec4<I32>& rect, const DELEGATE_CBK& callback);

    //returns an immediate mode emulation buffer that can be used to construct geometry in a vertex by vertex manner.
    //allowPrimitiveRecycle = do not reuse old primitives and do not delete it after x-frames. (Don't use the primitive zombie feature)
    IMPrimitive* getOrCreatePrimitive(bool allowPrimitiveRecycle = true);

    inline void setInterpolation(const D32 interpolation)       {_interpolationFactor = interpolation; }
    inline D32  getInterpolation()                        const {return _interpolationFactor;} 
    inline void drawDebugAxis(const bool state)                 {_drawDebugAxis = state;}
    inline bool drawDebugAxis()                           const {return _drawDebugAxis;}

    void debugDraw(const SceneRenderState& sceneRenderState);
    void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const vec4<U8>& color);
    void drawLines(const ::vectorImpl<Line >& lines,
                   const mat4<F32>& globalOffset,
                   const vec4<I32>& viewport, //<only for ortho mode
                   const bool inViewport = false,
                   const bool disableDepth = false);
    
    void drawPoints(U32 numPoints, size_t stateHash, ShaderProgram* const shaderProgram);
    void drawGUIElement(GUIElement* guiElement);
    void submitRenderCommand(VertexDataInterface* const buffer, const GenericDrawCommand& cmd);
    void submitRenderCommand(VertexDataInterface* const buffer, const ::vectorImpl<GenericDrawCommand>& cmds);
    /// returns false if there was an invalid state detected that could prevent rendering
    bool setBufferData(const GenericDrawCommand& cmd);

    inline I32 getDrawID(I64 drawIDIndex) {
        assert(_sgnToDrawIDMap.find(drawIDIndex) != _sgnToDrawIDMap.end());
        return _sgnToDrawIDMap[drawIDIndex];
    }
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

    Renderer* getRenderer() const;
    void setRenderer(Renderer* const renderer);

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
    /// 2D rendering enabled
    inline bool is2DRendering()                  const { return _2DRendering; }
    ///Save a screenshot in TGA format
    void Screenshot(char *filename);
    /// Some Scene Node Types are excluded from certain operations (lights triggers, etc)
    inline bool excludeFromStateChange(const SceneNodeType& currentType) {
        return (_stateExclusionMask & currentType) == currentType ? true : false;
    }
    inline void setStateChangeExclusionMask(I32 stateMask) {_stateExclusionMask = stateMask; }
    /// Return or create a new state block using the given descriptor. DO NOT DELETE THE RETURNED STATE BLOCK! GFXDevice handles that!
    size_t getOrCreateStateBlock(const RenderStateBlockDescriptor& descriptor);
    const RenderStateBlockDescriptor& getStateBlockDescriptor(size_t renderStateBlockHash) const;
    ///returns the standard state block
    inline size_t getDefaultStateBlock(bool noDepth = false)      { return noDepth ? _defaultStateNoDepthHash : _defaultStateBlockHash; } 
    /*//Render State Management */

    ///Generate a cubemap from the given position
    ///It renders the entire scene graph (with culling) as default
    ///use the callback param to override the draw function
    void  generateCubeMap(Framebuffer& cubeMap,
                          const vec3<F32>& pos,
                          const DELEGATE_CBK& renderFunction, 
                          const vec2<F32>& zPlanes,
                          const RenderStage& renderStage = REFLECTION_STAGE);

    void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat);

    inline ShaderProgram* activeShaderProgram()                             const { return _activeShaderProgram; }
    inline void           activeShaderProgram(ShaderProgram* const program)       { _activeShaderProgram = program; }

    inline const mat4<F32>& getMatrix(const MATRIX_MODE& mode)  { getMatrix(mode, _mat4Cache); return _mat4Cache; }
    
    inline U64  getFrameDurationGPU()      { return _api.getFrameDurationGPU(); }
    inline void togglePreviewDepthBuffer() { _previewDepthBuffer = !_previewDepthBuffer; }

    inline bool MSAAEnabled() const { return _MSAASamples > 0; }
    inline U8   MSAASamples() const { return _MSAASamples; }
    inline bool FXAAEnabled() const { return _FXAASamples > 0; }
    inline U8   FXAASamples() const { return _FXAASamples; }

    RenderDetailLevel generalDetailLevel()                             const { return _generalDetailLevel; }
    void              generalDetailLevel(RenderDetailLevel detailLevel)      { _generalDetailLevel = detailLevel; }
    RenderDetailLevel shadowDetailLevel()                              const { return _shadowDetailLevel; }
    void              shadowDetailLevel(RenderDetailLevel detailLevel)       { _shadowDetailLevel = detailLevel; }

    inline void add2DRenderFunction(const DELEGATE_CBK& callback, U32 callOrder);

    void restoreViewport();
    void setViewport(const vec4<I32>& viewport);

    bool loadInContext(const CurrentContext& context, const DELEGATE_CBK& callback);

    void ConstructHIZ();

    void processVisibleNodes(const ::vectorImpl<SceneGraphNode* >& visibleNodes);

    inline U32  getFrameCount()       const { return FRAME_COUNT; }
    inline I32  getDrawCallCount()    const { return FRAME_DRAW_CALLS_PREV; }
    inline void registerDrawCall()          { FRAME_DRAW_CALLS++; }
    inline bool loadingThreadAvailable() const { return _loadingThreadAvailable && _loaderThread;  }

    inline U32 getMaxTextureSlots()        const { return _maxTextureSlots; }
    inline U32 getMaxRenderTargetOutputs() const { return _maxRenderTargetOutputs; }

protected:
    /// Some functions should only be accessable from the rendering api itself
    friend class GL_API;
    friend class DX_API;

    inline LoadQueue& getLoadQueue() { return _loadQueue; }

    inline void initAA(U8 fxaaSamples, U8 msaaSamples){
        _FXAASamples = fxaaSamples;
        _MSAASamples = msaaSamples;
    }

    /// register a new display mode (resolution, bitdepth, etc).
    inline void registerDisplayMode(const GPUVideoMode& mode) {
        // this is terribly slow, but should only be called a couple of times and only on video hardware init
        if (std::find(_supportedDislpayModes.begin(), _supportedDislpayModes.end(), mode) != _supportedDislpayModes.end()) {
            _supportedDislpayModes.push_back(mode);
        }
    }

    inline void loadingThreadAvailable(bool state)                    { _loadingThreadAvailable = state; }
    inline void setMaxTextureSlots(U32 textureSlots)                  { _maxTextureSlots = textureSlots; }
    inline void setMaxRenderTargetOutputs(U32 maxRenderTargetOutputs) { _maxRenderTargetOutputs = maxRenderTargetOutputs; }

protected:
    friend class Kernel;
    friend class Application;
    inline void setCursorPosition(U16 x, U16 y)                 const {_api.setCursorPosition(x,y);}
    inline void changeResolutionInternal(U16 width, U16 height)       { _api.changeResolutionInternal(width, height); }
    inline void changeViewport(const vec4<I32>& newViewport)    const { _api.changeViewport(newViewport); }
    inline void createLoaderThread()                                  { _api.createLoaderThread(); }
    inline void drawPoints(U32 numPoints)                             { _api.drawPoints(numPoints); }
           void drawDebugAxis(const SceneRenderState& sceneRenderState);

protected:
    friend class Camera;

    F32* lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& eyePos);
    ///sets an ortho projection, updating any listeners if needed
    F32* setProjection(const vec4<F32>& rect, const vec2<F32>& planes);
    ///sets a perspective projection, updating any listeners if needed
    F32* setProjection(F32 FoV, F32 aspectRatio, const vec2<F32>& planes) ;
    ///sets the view frustum to either the left or right eye position for anaglyph rendering
    void setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes, F32 aspectRatio, F32 verticalFoV, bool rightFrustum = false);

private:

    GFXDevice();
    ~GFXDevice();
    void previewDepthBuffer();
    void updateViewportInternal(const vec4<I32>& viewport);
    void forceViewportInternal(const vec4<I32>& viewport);
    ///Update the graphics pipeline using the current rendering API with the state block passed
    inline void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock)  const { _api.activateStateBlock(newBlock, oldBlock); }
    inline void drawText(const TextLabel& textLabel, const vec2<I32>& position) { _api.drawText(textLabel, position); }
    ///Sets the current state block to the one passed as a param
    size_t setStateBlock(size_t stateBlockHash);

private:
    Camera*           _cubeCamera;
    Camera*           _2DCamera;
    PostFX&           _postFX;
    ShaderManager&    _shaderManager;
    RenderAPIWrapper& _api;
    RenderStage _renderStage;
    U32  _maxTextureSlots;
    U32  _maxRenderTargetOutputs;
    U32  _prevShaderId,  _prevTextureId;
    I32  _stateExclusionMask;
    bool _drawDebugAxis;
    bool _viewportUpdate;
    LoadQueue _loadQueue;
    boost::atomic_bool _loadingThreadAvailable;
    boost::thread *_loaderThread;
    ShaderProgram* _activeShaderProgram;
     
    ::vectorImpl<Line > _axisLines;
    ::vectorImpl<Line > _axisLinesTrasnformed;

protected:
    Renderer* _renderer;
    /* Rendering buffers*/
    Framebuffer* _renderTarget[RenderTarget_PLACEHOLDER];
    /*State management */
    RenderStateMap _stateBlockMap;
    bool _stateBlockByDescription;
    size_t _currentStateBlockHash;
    size_t _previousStateBlockHash;
    size_t _defaultStateBlockHash;
    size_t _defaultStateNoDepthHash; //<The default render state buth with depth testing disabled
    size_t _state2DRenderingHash;    //<Special render state for 2D rendering
    size_t _stateDepthOnlyRenderingHash;
    mat4<F32> _textureMatrix;
    ///The interpolation factor between the current and the last frame
    D32       _interpolationFactor;
    ///Line width management 
    F32 _previousLineWidth, _currentLineWidth;
    ///Pointer to current kernel
    Kernel*   _kernel;
    PlaneList _clippingPlanes;
    bool      _enablePostProcessing;
    bool      _enableAnaglyph;
    bool      _2DRendering;
    bool      _rasterizationEnabled;
   
    // number of draw calls (rough estimate)
    I32 FRAME_DRAW_CALLS;
    U32 FRAME_DRAW_CALLS_PREV;
    U32 FRAME_COUNT;
    ///shader used to preview the depth buffer
    ShaderProgram* _previewDepthMapShader;
    ShaderProgram* _HIZConstructProgram;
    bool    _previewDepthBuffer;
    ///getMatrix cache
    mat4<F32> _mat4Cache;
    /// AA system
    U8        _MSAASamples;
    U8        _FXAASamples;
    /// Quality settings
    RenderDetailLevel _generalDetailLevel;
    RenderDetailLevel _shadowDetailLevel;

    vectorImpl<std::pair<U32, DELEGATE_CBK> > _2dRenderQueue;

    ///Immediate mode emulation
    ShaderProgram*             _imShader;     //<The shader used to render VB data
    vectorImpl<IMPrimitive* >  _imInterfaces; //<The interface that coverts IM calls to VB data

    ///Current viewport stack
    ViewportStack _viewport;

    GPUBlock                _gpuBlock;

    ::vectorImpl<NodeData >     _matricesData;
    ::vectorImpl<GPUVideoMode > _supportedDislpayModes;
    Unordered_map<I64, I32>   _sgnToDrawIDMap;

    ShaderBuffer*  _gfxDataBuffer;
    ShaderBuffer*  _nodeBuffer;
  
    GenericDrawCommand _defaultDrawCmd;

END_SINGLETON

}; //namespace Divide

#include "GFXDevice-Inl.h"

#endif
