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

#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Direct3D/Headers/DXWrapper.h"
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
DEFINE_SINGLETON_EXT1(GFXDevice, RenderAPIWrapper)
    typedef hashMapImpl<size_t, RenderStateBlock* > RenderStateMap;
    typedef std::stack<vec4<I32>, vectorImpl<vec4<I32> > > ViewportStack;
    typedef boost::lockfree::spsc_queue<DELEGATE_CBK<>, boost::lockfree::capacity<15> > LoadQueue;

public:
    struct GPUBlock {
        mat4<F32> _ProjectionMatrix;
        mat4<F32> _ViewMatrix;
        mat4<F32> _ViewProjectionMatrix;
        vec4<F32> _cameraPosition;
        vec4<F32> _ViewPort;
        vec4<F32> _ZPlanesCombined; //xy - current, zw - main scene 
        vec4<F32> _clipPlanes[Config::MAX_CLIP_PLANES];
    };

    enum RenderTarget {
        RENDER_TARGET_SCREEN = 0,
        RENDER_TARGET_ANAGLYPH = 1,
        RENDER_TARGET_DEPTH = 2,
        RenderTarget_PLACEHOLDER = 3
    };

public:
    ErrorCode initRenderingApi(const vec2<U16>& resolution, I32 argc, char **argv);
    void closeRenderingApi();

    void idle();
    void beginFrame();
    void endFrame();
        
    void changeResolution(U16 w, U16 h);
    void enableFog(F32 density, const vec3<F32>& color);
    void toggle2D(bool state);
    /// Toggle hardware rasterization on or off. 
    inline void toggleRasterization(bool state);
    /// Change the width of rendered lines to the specified value
    inline void setLineWidth(F32 width);
    /// Restore the width of rendered lines to the previously set value
    inline void restoreLineWidth();
    /// Render specified function inside of a viewport of specified dimensions and position
    inline void renderInViewport(const vec4<I32>& rect, const DELEGATE_CBK<>& callback);

    /**
     *@brief Returns an immediate mode emulation buffer that can be used to construct geometry in a vertex by vertex manner.
     *@param allowPrimitiveRecycle If false, do not reuse old primitives and do not delete it after x-frames. 
     *(Don't use the primitive zombie feature)
     */
    IMPrimitive* getOrCreatePrimitive(bool allowPrimitiveRecycle = true);

    inline void setInterpolation(const D32 interpolation)       {_interpolationFactor = interpolation; }
    inline D32  getInterpolation()                        const {return _interpolationFactor;} 

    inline void             setApi(const RenderAPI& apiId)       { _apiId = apiId; }
    inline const RenderAPI& getApi()                       const { return _apiId; }

    inline void             setGPUVendor(const GPUVendor& gpuvendor)       { _GPUVendor = gpuvendor; }
    inline const GPUVendor& getGPUVendor()                           const { return _GPUVendor; }

    inline void drawDebugAxis(const bool state)       {_drawDebugAxis = state;}
    inline bool drawDebugAxis()                 const {return _drawDebugAxis;}

    void debugDraw(const SceneRenderState& sceneRenderState);
    void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const vec4<U8>& color);
    void drawLines(const vectorImpl<Line >& lines,
                   const mat4<F32>& globalOffset,
                   const vec4<I32>& viewport, //<only for ortho mode
                   const bool inViewport = false,
                   const bool disableDepth = false);
    
    void drawPoints(U32 numPoints, size_t stateHash, ShaderProgram* const shaderProgram);
    void drawGUIElement(GUIElement* guiElement);
    void submitRenderCommand(const GenericDrawCommand& cmd);
    void submitRenderCommand(const vectorImpl<GenericDrawCommand>& cmds);
    ///Sets the current render stage.
    ///@param stage Is used to inform the rendering pipeline what we are rendering. Shadows? reflections? etc
    inline RenderStage setRenderStage(RenderStage stage);
    inline RenderStage getRenderStage()                  const { return _renderStage; }
    ///Checks if the current rendering stage is any of the stages defined in renderStageMask
    ///@param renderStageMask Is a bitmask of the stages we whish to check if active
    inline bool isCurrentRenderStage(U8 renderStageMask);
    /// Some Scene Node Types are excluded from certain operations (lights triggers, etc)
    inline bool excludeFromStateChange(const SceneNodeType& currentType) { 
        return (_stateExclusionMask & currentType) == currentType; 
    }
    inline void setStateChangeExclusionMask(I32 stateMask) { _stateExclusionMask = stateMask; }

    inline void setPrevShaderId(const U32& id)  { _prevShaderId = id; }
    inline U32  getPrevShaderId()               { return _prevShaderId; }

    inline void setPrevTextureId(const U32& id) { _prevTextureId = id; }
    inline U32  getPrevTextureId()              { return _prevTextureId; }

    void      setRenderer(Renderer* const renderer);
    Renderer* getRenderer() const;
        
    /// Clipping plane management. All the clipping planes are handled by shader programs only!
    void updateClipPlanes();
    /// disable or enable a clip plane by index
    inline void toggleClipPlane(ClipPlaneIndex index, const bool state);
    /// modify a single clip plane by index
    inline void setClipPlane(ClipPlaneIndex index, const Plane<F32>& p);
    /// set a new list of clipping planes. The old one is discarded
    inline void setClipPlanes(const PlaneList& clipPlanes);
    /// clear all clipping planes
    inline void resetClipPlanes();
    /// get the entire list of clipping planes
    inline const PlaneList& getClippingPlanes() const { return _clippingPlanes; }
    /// 2D rendering enabled
    inline bool is2DRendering() const { return _2DRendering; }
    /// Post Processing state
    inline bool postProcessingEnabled() const { return _enablePostProcessing; }
           void postProcessingEnabled(const bool state);
    /// Anaglyph rendering state
    inline bool anaglyphEnabled() const { return _enableAnaglyph; }
    inline void anaglyphEnabled(const bool state) { _enableAnaglyph = state; }
 
    /// Return or create a new state block using the given descriptor. 
    /// DO NOT DELETE THE RETURNED STATE BLOCK! GFXDevice handles that!
    size_t getOrCreateStateBlock(const RenderStateBlockDescriptor& descriptor);
    const RenderStateBlockDescriptor& getStateBlockDescriptor(size_t renderStateBlockHash) const;
    ///returns the standard state block
    inline size_t getDefaultStateBlock(bool noDepth = false) { 
        return noDepth ? _defaultStateNoDepthHash : 
                         _defaultStateBlockHash; 
    } 
    inline Framebuffer* getRenderTarget(RenderTarget target)  const { return _renderTarget[target]; }

    ///Generate a cubemap from the given position
    ///It renders the entire scene graph (with culling) as default
    ///use the callback param to override the draw function
    void  generateCubeMap(Framebuffer& cubeMap,
                          const vec3<F32>& pos,
                          const DELEGATE_CBK<>& renderFunction,
                          const vec2<F32>& zPlanes,
                          const RenderStage& renderStage = REFLECTION_STAGE);

    void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat);
    /// Alternative to the normal version of getMatrix
    inline const mat4<F32>& getMatrix(const MATRIX_MODE& mode);

    inline ShaderProgram* activeShaderProgram()                             const { return _activeShaderProgram; }
    inline void           activeShaderProgram(ShaderProgram* const program)       { _activeShaderProgram = program; }
        
    inline bool MSAAEnabled() const { return _MSAASamples > 0; }
    inline U8   MSAASamples() const { return _MSAASamples; }
    inline bool FXAAEnabled() const { return _FXAASamples > 0; }
    inline U8   FXAASamples() const { return _FXAASamples; }

    RenderDetailLevel generalDetailLevel()                             const { return _generalDetailLevel; }
    void              generalDetailLevel(RenderDetailLevel detailLevel)      { _generalDetailLevel = detailLevel; }
    RenderDetailLevel shadowDetailLevel()                              const { return _shadowDetailLevel; }
    void              shadowDetailLevel(RenderDetailLevel detailLevel)       { _shadowDetailLevel = detailLevel; }
    /// Register a function to be called in the 2D rendering fase of the GFX Flush routine. Use callOrder for sorting purposes 
    inline void add2DRenderFunction(const DELEGATE_CBK<>& callback, U32 callOrder);

    void restoreViewport();
    void setViewport(const vec4<I32>& viewport);
    inline const vec4<I32>& getCurrentViewport()  const { return _viewport.top(); }

    bool loadInContext(const CurrentContext& context, const DELEGATE_CBK<>& callback);

    void ConstructHIZ();
    ///Save a screenshot in TGA format
    void Screenshot(char *filename);

    inline U32  getFrameCount()          const { return FRAME_COUNT; }
    inline I32  getDrawCallCount()       const { return FRAME_DRAW_CALLS_PREV; }
    inline void registerDrawCall()             { FRAME_DRAW_CALLS++; }
    inline bool loadingThreadAvailable() const { return _loadingThreadAvailable && _loaderThread;  }

    /// Direct API function delegates
    inline void setWindowPos(U16 w, U16 h) const { _api->setWindowPos(w, h); }
    /// Hardware specific shader preps (e.g.: OpenGL: init/deinit GLSL-OPT and GLSW)
    inline bool initShaders()   { return _api->initShaders(); }
    inline bool deInitShaders() { return _api->deInitShaders(); }
    inline IMPrimitive*        newIMP()                                      const { return _api->newIMP(); }
    inline Framebuffer*        newFB(bool multisampled = false)              const { return _api->newFB(multisampled); }
    inline VertexBuffer*       newVB()                                       const { return _api->newVB(); }
    inline PixelBuffer*        newPB(const PBType& type = PB_TEXTURE_2D)     const { return _api->newPB(type); }
    inline GenericVertexData*  newGVD(const bool persistentMapped)           const { return _api->newGVD(persistentMapped); }
    inline Texture*            newTextureArray(const bool flipped = false)   const { return _api->newTextureArray(flipped); }
    inline Texture*            newTexture2D(const bool flipped = false)      const { return _api->newTexture2D(flipped); }
    inline Texture*            newTextureCubemap(const bool flipped = false) const { return _api->newTextureCubemap(flipped); }
    inline ShaderProgram*      newShaderProgram(const bool optimise = false) const { return _api->newShaderProgram(optimise); }
    inline Shader*             newShader(const stringImpl& name, const  ShaderType& type, const bool optimise = false) const {
        return _api->newShader(name, type, optimise);
    }
    inline ShaderBuffer* newSB(const bool unbound = false, const bool persistentMapped = true) const {
        return _api->newSB(unbound, persistentMapped);
    }
    inline U64  getFrameDurationGPU()  { return _api->getFrameDurationGPU(); }

protected:
    /// Some functions should only be accessable from the rendering api itself
    friend class GL_API;
    friend class DX_API;

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

protected:

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

    inline void loadingThreadAvailable(bool state)  { _loadingThreadAvailable = state; }
    inline void uploadDrawCommands(const vectorImpl<IndirectDrawCommand>& drawCommands) const { 
        _api->uploadDrawCommands(drawCommands); 
    }

protected:
    friend class Kernel;
    friend class Application;
    inline void setCursorPosition(U16 x, U16 y)                 const { _api->setCursorPosition(x,y);}
    inline void changeResolutionInternal(U16 width, U16 height)       { _api->changeResolutionInternal(width, height); }
    inline void changeViewport(const vec4<I32>& newViewport)    const { _api->changeViewport(newViewport); }
    inline void createLoaderThread()                                  { _api->createLoaderThread(); }
    inline void drawPoints(U32 numPoints)                             { _api->drawPoints(numPoints); }
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

protected:
    friend class RenderPassCuller;
    void processVisibleNodes(const vectorImpl<SceneGraphNode* >& visibleNodes);
    void buildDrawCommands(const vectorImpl<SceneGraphNode* >& visibleNodes, SceneRenderState& sceneRenderState);

private:

    GFXDevice();
    ~GFXDevice();
    void previewDepthBuffer();
    void updateViewportInternal(const vec4<I32>& viewport);
    void forceViewportInternal(const vec4<I32>& viewport);
    /// returns false if there was an invalid state detected that could prevent rendering
    bool setBufferData(const GenericDrawCommand& cmd);
    ///Update the graphics pipeline using the current rendering API with the state block passed
    inline void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock) const { 
        _api->activateStateBlock(newBlock, oldBlock); 
    }
    inline void drawText(const TextLabel& textLabel, const vec2<I32>& position) { 
        _api->drawText(textLabel, position); 
    }
    ///Sets the current state block to the one passed as a param
    size_t setStateBlock(size_t stateBlockHash);
    ErrorCode createAPIInstance();

private:
    Camera*           _cubeCamera;
    Camera*           _2DCamera;
    RenderAPIWrapper* _api;
    RenderStage _renderStage;
    U32  _prevShaderId,  _prevTextureId;
    I32  _stateExclusionMask;
    bool _drawDebugAxis;
    bool _viewportUpdate;
    LoadQueue _loadQueue;
    std::atomic_bool _loadingThreadAvailable;
    std::thread *_loaderThread;
    ShaderProgram* _activeShaderProgram;
     
    vectorImpl<Line > _axisLines;
    vectorImpl<Line > _axisLinesTrasnformed;

protected:

    struct NodeData {
        mat4<F32> _matrix[4];

        NodeData()
        {
            _matrix[0].identity();
            _matrix[1].identity();
            _matrix[2].zero();
            _matrix[3].zero();
        }
    };

protected:

    RenderAPI _apiId;
    GPUVendor _GPUVendor;
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
    ///getMatrix cache
    mat4<F32> _mat4Cache;
    /// AA system
    U8        _MSAASamples;
    U8        _FXAASamples;
    /// Quality settings
    RenderDetailLevel _generalDetailLevel;
    RenderDetailLevel _shadowDetailLevel;

    vectorImpl<std::pair<U32, DELEGATE_CBK<> > > _2dRenderQueue;

    ///Immediate mode emulation
    ShaderProgram*             _imShader;     //<The shader used to render VB data
    vectorImpl<IMPrimitive* >  _imInterfaces; //<The interface that coverts IM calls to VB data

    ///Current viewport stack
    ViewportStack _viewport;

    GPUBlock                _gpuBlock;

    vectorImpl<NodeData >     _matricesData;
    vectorImpl<GPUVideoMode > _supportedDislpayModes;

    ShaderBuffer*  _gfxDataBuffer;
    ShaderBuffer*  _nodeBuffer;
  
    GenericDrawCommand _defaultDrawCmd;

END_SINGLETON

}; //namespace Divide

#include "GFXDevice.inl"


#endif