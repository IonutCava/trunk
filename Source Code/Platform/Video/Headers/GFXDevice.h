/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _HARDWARE_VIDEO_GFX_DEVICE_H_
#define _HARDWARE_VIDEO_GFX_DEVICE_H_

#include "GFXState.h"
#include "ScopedStates.h"

#include "Managers/Headers/RenderPassManager.h"

#include "Platform/Video/Headers/RenderAPIWrapper.h"

#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

#include <stack>
#include <future>

namespace Divide {

enum class RenderStage : U32;
enum class RendererType : U32;
enum class SceneNodeType : U32;
enum class WindowEvent : U32;

class GUI;
class GUIText;

class Light;
class Camera;
class PostFX;
class Quad3D;
class Object3D;
class Renderer;
class SceneGraphNode;
class SceneRenderState;

namespace Time {
    class ProfileTimer;
};

namespace Attorney {
    class GFXDeviceGUI;
    class GFXDeviceKernel;
    class GFXDeviceRenderer;
    class GFXDeviceRenderStateBlock;
};

/// Rough around the edges Adapter pattern abstracting the actual rendering API
/// and access to the GPU
DEFINE_SINGLETON(GFXDevice)
    friend class Attorney::GFXDeviceGUI;
    friend class Attorney::GFXDeviceKernel;
    friend class Attorney::GFXDeviceRenderer;
    friend class Attorney::GFXDeviceRenderStateBlock;
  protected:
    typedef hashMapImpl<U32, RenderStateBlock> RenderStateMap;
    typedef std::stack<vec4<I32>> ViewportStack;

    struct NodeData {
        mat4<F32> _worldMatrix;
        mat4<F32> _normalMatrixWV;
        mat4<F32> _colorMatrix;
        vec4<F32> _properties;

        NodeData()
        {
            _worldMatrix.identity();
            _normalMatrixWV.identity();
            _colorMatrix.zero();
        }
        void set(const NodeData& other);
    };

  public:  // GPU specific data
   struct ShaderBufferBinding {
       ShaderBufferLocation _slot;
       ShaderBuffer* _buffer;
       vec2<U32>    _range;

       ShaderBufferBinding() : _buffer(nullptr)
       {
       }

       ShaderBufferBinding(ShaderBufferLocation slot,
                           ShaderBuffer* buffer,
                           const vec2<U32>& range) 
         : _slot(slot),
           _buffer(buffer),
           _range(range)
       {
       }

       void set(const ShaderBufferBinding& other);

       void set(ShaderBufferLocation slot,
                ShaderBuffer* buffer,
                const vec2<U32>& range);
   };

   typedef vectorImpl<ShaderBufferBinding> ShaderBufferList;

   struct RenderPackage {
       RenderPackage() : _isRenderable(false)
       {
       }

       bool isCompatible(const RenderPackage& other) const;

       void clear();
       void set(const RenderPackage& other);

       inline bool isRenderable() const {
           return  _isRenderable;
       }

       inline void isRenderable(bool state) {
           _isRenderable = state;
       }

       ShaderBufferList _shaderBuffers;
       TextureDataContainer _textureData;
       vectorImpl<GenericDrawCommand> _drawCommands;

       private:
           bool _isRenderable;
   };

   struct RenderQueue {
    public:
       RenderQueue() : _currentCount(0)
       {
       }
       void clear();
       U32 size() const;
       bool empty() const;
       const RenderPackage& getPackage(U32 idx) const;

    protected:
        friend class GFXDevice;
        RenderPackage& getPackage(U32 idx);
        RenderPackage& back();
        bool push_back(const RenderPackage& package);
        void resize(U16 size);

    protected:
       U32 _currentCount;
       vectorImpl<RenderPackage> _packages;
   };

   struct DisplaySettings {
       F32 _aspectRatio;
       F32 _FoV;
       F32 _tanHFoV;
       vec2<F32> _zPlanes;
       mat4<F32> _projectionMatrix;
       mat4<F32> _viewMatrix;

       DisplaySettings()
       {
           _aspectRatio = 1.0f;
           _FoV = 90;
           _tanHFoV = std::tan(_FoV * 0.5f);
           _zPlanes.set(0.1f, 1.0f);
       }
   };

   struct RenderTarget {
       Framebuffer* _buffer;
       DisplaySettings _renderSettings;

       RenderTarget() : _buffer(nullptr)
       {
       }

       void cacheSettings();
   };

   enum class RenderAPI : U32 {
       OpenGL,    ///< 4.x+
       OpenGLES,  ///< 3.x+
       Direct3D,  ///< 11.x+ (not supported yet)
       Vulkan,    ///< not supported yet
       None,      ///< not supported yet
       COUNT
   };

   enum class RenderTargetID : U32 {
       SCREEN = 0,
       ANAGLYPH = 1,
       ENVIRONMENT = 2,
       COUNT
   };

   // Enough to hold a cubemap (e.g. environment stage has 6 passes)
   static const U32 MAX_PASSES_PER_STAGE = 6;

   struct GPUBlock {
       GPUBlock() : _updated(true)
       {
       }

       struct GPUData {
           mat4<F32> _ProjectionMatrix;
           mat4<F32> _ViewMatrix;
           mat4<F32> _ViewProjectionMatrix;
           vec4<F32> _cameraPosition; // xyz - position, w - aspect ratio
           vec4<F32> _ViewPort;
           vec4<F32> _ZPlanesCombined;  // xy - current, zw - main scene
           vec4<F32> _invScreenDimension; //xy - dims, zw - reserved;
           vec4<F32> _renderProperties;
           vec4<F32> _clipPlanes[Config::MAX_CLIP_PLANES];

           inline F32 aspectRatio() const;
           inline vec2<F32> currentZPlanes() const;
           inline F32 FoV() const;
           inline F32 tanHFoV() const;

        } _data;

        bool _updated;
   };

  public:  // GPU interface
    ErrorCode initRenderingAPI(I32 argc, char** argv);
    void closeRenderingAPI();

    inline void setAPI(RenderAPI API) { _API_ID = API; }
    inline RenderAPI getAPI() const { return _API_ID; }

    void idle();
    void beginFrame();
    void endFrame();
    void handleWindowEvent(WindowEvent event, I32 data1, I32 data2);

    /// Set all of the needed API specific settings for 2D (Ortho) / 3D
    /// (Perspective) rendering
    void toggle2D(bool state);
    /// Toggle writes to the depth buffer on or off
    inline void toggleDepthWrites(bool state);
    /// Toggle hardware rasterization on or off.
    inline void toggleRasterization(bool state);
    /// Query rasterization state
    inline bool rasterizationState();

    /**
     *@brief Returns an immediate mode emulation buffer that can be used to
     *construct geometry in a vertex by vertex manner.
     *@param allowPrimitiveRecycle If false, do not reuse old primitives and do not
     *delete it after x-frames.
     *(Don't use the primitive zombie feature)
     */
    IMPrimitive* getOrCreatePrimitive(bool allowPrimitiveRecycle = true);

    void debugDraw(const SceneRenderState& sceneRenderState);
    void drawSphere3D(IMPrimitive& primitive,
                      const vec3<F32>& center,
                      F32 radius,
                      const vec4<U8>& color);
    void drawBox3D(IMPrimitive& primitive,
                   const vec3<F32>& min,
                   const vec3<F32>& max,
                   const vec4<U8>& color);
    void drawLines(IMPrimitive& primitive,
                   const vectorImpl<Line>& lines,
                   const vec4<I32>& viewport,  //<only for ortho mode
                   const bool inViewport = false);
    void drawPoints(U32 numPoints, U32 stateHash, ShaderProgram* const shaderProgram);
    void drawTriangle(U32 stateHash, ShaderProgram* const shaderProgram);

    void addToRenderQueue(U32 binPropertyMask, const RenderPackage& package);
    void flushRenderQueue();

    /// Sets the current render stage.
    ///@param stage Is used to inform the rendering pipeline what we are rendering.
    ///Shadows? reflections? etc
    inline bool isDepthStage() const;

    /// Clipping plane management. All the clipping planes are handled by shader
    /// programs only!
    void updateClipPlanes();
    /// disable or enable a clip plane by index
    inline void toggleClipPlane(ClipPlaneIndex index, const bool state);
    /// modify a single clip plane by index
    inline void setClipPlane(ClipPlaneIndex index, const Plane<F32>& p);
    /// set a new list of clipping planes. The old one is discarded
    inline void setClipPlanes(const PlaneList& clipPlanes);
    /// clear all clipping planes
    inline void resetClipPlanes();
    /// Retrieve a state block by hash value.
    /// If the hash value doesn't exist in the state block map, return the default state block
    const RenderStateBlock& getRenderStateBlock(U32 renderStateBlockHash) const;

    /// Generate a cubemap from the given position
    /// It renders the entire scene graph (with culling) as default
    /// use the callback param to override the draw function
    void generateCubeMap(Framebuffer& cubeMap,
                         const U32 arrayOffset,
                         const vec3<F32>& pos,
                         const vec2<F32>& zPlanes,
                         RenderStage renderStage);

    void generateDualParaboloidMap(Framebuffer& targetBuffer,
                                   const U32 arrayOffset,
                                   const vec3<F32>& pos,
                                   const vec2<F32>& zPlanes,
                                   RenderStage renderStage);
                                     
    void getMatrix(const MATRIX& mode, mat4<F32>& mat);
    /// Alternative to the normal version of getMatrix
    inline const mat4<F32>& getMatrix(const MATRIX& mode);
    /// Access (Read Only) rendering data used by the GFX
    inline const GPUBlock::GPUData& renderingData() const;
    /// Register a function to be called in the 2D rendering fase of the GFX Flush
    /// routine. Use callOrder for sorting purposes
    inline void add2DRenderFunction(const DELEGATE_CBK<>& callback, U32 callOrder);

    void restoreViewport();
    void setViewport(const vec4<I32>& viewport);
    inline void setViewport(I32 x, I32 y, I32 width, I32 height);
    /// Switch between fullscreen rendering
    void toggleFullScreen();
    void increaseResolution();
    void decreaseResolution();
    void changeResolution(U16 w, U16 h);
    bool loadInContext(const CurrentContext& context,
                       const DELEGATE_CBK<>& callback);

    /// Save a screenshot in TGA format
    void Screenshot(const stringImpl& filename);

  public:  // Accessors and Mutators
    inline const GPUState& gpuState() const { return _state; }

    inline GPUState& gpuState() { return _state; }

    inline void setInterpolation(const D32 interpolation) {
        _interpolationFactor = interpolation;
    }

    inline D32 getInterpolation() const { return _interpolationFactor; }

    inline void setGPUVendor(const GPUVendor& gpuvendor) { _GPUVendor = gpuvendor; }

    inline const GPUVendor& getGPUVendor() const { return _GPUVendor; }

    inline void drawDebugAxis(const bool state) { _drawDebugAxis = state; }

    inline bool drawDebugAxis() const { return _drawDebugAxis; }

    inline RenderStage getRenderStage() const { return _renderStage; }
    inline RenderStage getPrevRenderStage() const { return _prevRenderStage; }

    /// Get the entire list of clipping planes
    inline const PlaneList& getClippingPlanes() const { return _clippingPlanes; }

    /// Return the last number of HIZ culled items
    U32 getLastCullCount(U32 pass = 0) const;

    /// 2D rendering enabled
    inline bool is2DRendering() const { return _2DRendering; }

    /// Anaglyph rendering state
    inline bool anaglyphEnabled() const { return _enableAnaglyph; }

    inline void anaglyphEnabled(const bool state) { _enableAnaglyph = state; }

    /// returns the standard state block
    inline U32 getDefaultStateBlock(bool noDepth) const {
        return (noDepth ? _defaultStateNoDepthHash : _defaultStateBlockHash);
    }

    inline RenderTarget& getRenderTarget(RenderTargetID target) {
        return _renderTarget[to_uint(target)];
    }
    
    RenderDetailLevel shadowDetailLevel() const { return _shadowDetailLevel; }

    void shadowDetailLevel(RenderDetailLevel detailLevel) {
        _shadowDetailLevel = detailLevel;
    }

    inline U32 getFrameCount() const { return FRAME_COUNT; }

    inline I32 getDrawCallCount() const { return FRAME_DRAW_CALLS_PREV; }

    inline void registerDrawCall() { registerDrawCalls(1); }

    inline void registerDrawCalls(U32 count) { FRAME_DRAW_CALLS += count; }

    inline const vec4<I32>& getCurrentViewport() const { return _viewport.top(); }

    inline RenderStage setRenderStage(RenderStage stage);

    void submitCommand(const GenericDrawCommand& cmd,
                       bool useIndirectRender = false);

    inline void submitCommands(const vectorImpl<GenericDrawCommand>& cmds,
                               bool useIndirectRender = false);
  public:  // Direct API calls
    /// Hardware specific shader preps (e.g.: OpenGL: init/deinit GLSL-OPT and GLSW)
    inline bool initShaders() { return _api->initShaders(); }

    inline bool deInitShaders() { return _api->deInitShaders(); }

    inline IMPrimitive* newIMP() { return _api->newIMP(*this); }

    inline Framebuffer* newFB(bool multisampled = false) {
        return _api->newFB(*this, multisampled);
    }

    inline VertexBuffer* newVB() {
        return _api->newVB(*this);
    }

    inline PixelBuffer* newPB(const PBType& type = PBType::PB_TEXTURE_2D) {
        return _api->newPB(*this, type);
    }

    inline GenericVertexData* newGVD(const bool persistentMapped) {
        return _api->newGVD(*this, persistentMapped);
    }

    inline Texture* newTexture(TextureType type, bool asyncLoad) {
        return _api->newTexture(*this, type, asyncLoad);
    }

    inline ShaderProgram* newShaderProgram(bool asyncLoad) {
        return _api->newShaderProgram(*this, asyncLoad);
    }

    inline Shader* newShader(const stringImpl& name,
                             const ShaderType& type,
                             const bool optimise = false) {
        return _api->newShader(*this, name, type, optimise);
    }

    inline ShaderBuffer* newSB(const stringImpl& bufferName,
                               const U32 ringBufferLength = 1,
                               const bool unbound = false,
                               const bool persistentMapped = true,
                               BufferUpdateFrequency frequency =
                                   BufferUpdateFrequency::ONCE) {
        return _api->newSB(*this, bufferName, ringBufferLength, unbound, persistentMapped, frequency);
    }

    inline U64 getFrameDurationGPU() {
        return _api->getFrameDurationGPU();
    }

    inline void setCursorPosition(I32 x, I32 y) {
        _api->setCursorPosition(x, y);
    }

  protected:
    void setBaseViewport(const vec4<I32>& viewport);

    inline void drawPoints(U32 numPoints) { 
        uploadGPUBlock();
        _api->drawPoints(numPoints); 
        registerDrawCall();
    }

    inline void drawTriangle() {
        uploadGPUBlock();
        _api->drawTriangle();
        registerDrawCall();
    }

    inline void drawText(const TextLabel& text,
                         U32 stateHash,
                         const vec2<F32>& relativeOffset) {
        uploadGPUBlock();
        setStateBlock(stateHash);
        _api->drawText(text, relativeOffset);
    }

    void drawDebugAxis(const SceneRenderState& sceneRenderState);

  protected:
    friend class Camera;

    F32* lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& eyePos);
    /// sets an ortho projection, updating any listeners if needed
    F32* setProjection(const vec4<F32>& rect, const vec2<F32>& planes);
    /// sets a perspective projection, updating any listeners if needed
    F32* setProjection(F32 FoV, F32 aspectRatio, const vec2<F32>& planes);
    /// sets the view frustum to either the left or right eye position for anaglyph
    /// rendering
    void setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes, F32 aspectRatio,
                            F32 verticalFoV, bool rightFrustum = false);

    void onCameraUpdate(Camera& camera);

    void flushDisplay();
    void flushAnaglyph();

   protected:
    friend class SceneManager;
    friend class RenderPass;
    void occlusionCull(U32 pass);
    void buildDrawCommands(RenderPassCuller::VisibleNodeList& visibleNodes,
                           SceneRenderState& sceneRenderState,
                           bool refreshNodeData,
                           U32 pass);
    bool batchCommands(GenericDrawCommand& previousIDC,
                       GenericDrawCommand& currentIDC) const;
    void constructHIZ();

   private:
    GFXDevice();
    ~GFXDevice();

    void previewDepthBuffer();
    void updateViewportInternal(const vec4<I32>& viewport);
    /// returns false if there was an invalid state detected that could prevent
    /// rendering
    bool setBufferData(const GenericDrawCommand& cmd);
    /// Upload draw related data to the GPU (view & projection matrices, viewport settings, etc)
    void uploadGPUBlock();

    /// If the stateBlock doesn't exist in the state block map, add it for future reference
    bool registerRenderStateBlock(const RenderStateBlock& stateBlock);

    /// Sets the current state block to the one passed as a param
    U32 setStateBlock(U32 stateBlockHash);
    ErrorCode createAPIInstance();

    NodeData& processVisibleNode(SceneGraphNode& node, U32 dataIndex);

    ShaderBuffer& getCommandBuffer(RenderStage stage, U32 pass) const;

    U32 getNodeBufferIndexForStage(RenderStage stage) const;

    ShaderBuffer& getNodeBuffer(RenderStage stage, U32 pass) const;

  private:
    Camera* _cubeCamera;
    Camera* _2DCamera;
    Camera* _dualParaboloidCamera;

    RenderAPIWrapper* _api;
    RenderStage _renderStage;
    RenderStage _prevRenderStage;
    bool _drawDebugAxis;
    bool _viewportUpdate;
    vectorImpl<Line> _axisLines;
    IMPrimitive     *_axisGizmo;
    vectorImpl<Line> _axisLinesTransformed;

  protected:
    RenderAPI _API_ID;
    GPUVendor _GPUVendor;
    GPUState _state;
    /* Rendering buffers*/
    std::array<RenderTarget, to_const_uint(RenderTargetID::COUNT)> _renderTarget;
    /*State management */
    RenderStateMap _stateBlockMap;
    bool _stateBlockByDescription;
    U32 _currentStateBlockHash;
    U32 _previousStateBlockHash;
    U32 _defaultStateBlockHash;
    /// The default render state buth with depth testing disabled
    U32 _defaultStateNoDepthHash;
    /// Special render state for 2D rendering
    U32 _state2DRenderingHash;
    U32 _stateDepthOnlyRenderingHash;
    /// The interpolation factor between the current and the last frame
    D32 _interpolationFactor;
    PlaneList _clippingPlanes;
    bool _enableAnaglyph;
    bool _2DRendering;
    bool _rasterizationEnabled;
    bool _zWriteEnabled;
    // number of draw calls (rough estimate)
    I32 FRAME_DRAW_CALLS;
    U32 FRAME_DRAW_CALLS_PREV;
    U32 FRAME_COUNT;
    /// shader used to preview the depth buffer
    ShaderProgram* _previewDepthMapShader;
    ShaderProgram* _framebufferDraw;
    ShaderProgram* _HIZConstructProgram;
    ShaderProgram* _HIZCullProgram;
    ShaderProgram* _displayShader;

    /// getMatrix cache
    mat4<F32> _mat4Cache;
    /// Quality settings
    RenderDetailLevel _shadowDetailLevel;

    vectorImpl<std::pair<U32, DELEGATE_CBK<> > > _2dRenderQueue;

    /// Immediate mode emulation shader
    ShaderProgram *_imShader, *_imShaderLines;
    I32 _imShaderTextureFlag;
    I32 _imShaderWorldMatrix;
    /// The interface that coverts IM calls to VB data
    vectorImpl<IMPrimitive*>  _imInterfaces;
    /// Current viewport stack
    ViewportStack _viewport;

    GPUBlock _gpuBlock;

    DrawCommandList _drawCommandsCache;
    std::array<NodeData, Config::MAX_VISIBLE_NODES> _matricesData;
    U32 _lastCommandCount;
    U32 _lastNodeCount;
    RenderQueue _renderQueue;
    Time::ProfileTimer* _commandBuildTimer;
    std::unique_ptr<ShaderBuffer> _gfxDataBuffer;
    typedef std::array<std::unique_ptr<ShaderBuffer>, MAX_PASSES_PER_STAGE> RenderStageBuffer;
    // Z_PRE_PASS and display SHOULD SHARE THE SAME BUFFERS
    std::array<RenderStageBuffer, to_const_uint(RenderStage::COUNT) - 1> _indirectCommandBuffers;
    std::array<RenderStageBuffer, to_const_uint(RenderStage::COUNT) - 1> _nodeBuffers;
    GenericDrawCommand _defaultDrawCmd;

END_SINGLETON

namespace Attorney {
    class GFXDeviceGUI {
    private:
        static void drawText(const TextLabel& text,
                             U32 stateHash,
                             const vec2<F32>& relativeOffset) {
            return GFXDevice::getInstance().drawText(text, stateHash, relativeOffset);
        }

        friend class Divide::GUI;
        friend class Divide::GUIText;
    };

    class GFXDeviceRenderStateBlock {
    private:
        static bool registerStateBlock(const RenderStateBlock& block) {
            return GFXDevice::getInstance().registerRenderStateBlock(block);
        }

        friend class Divide::RenderStateBlock;
    };

    class GFXDeviceKernel {
    private:
        static void onCameraUpdate(Camera& camera) {
            GFXDevice::getInstance().onCameraUpdate(camera);
        }

        static void flushDisplay() {
            GFXDevice::getInstance().flushDisplay();
        }

        static void flushAnaglyph() {
            GFXDevice::getInstance().flushAnaglyph();
        }

        friend class Divide::Kernel;
    };

    class GFXDeviceRenderer {
        private:
        static void uploadGPUBlock() {
            GFXDevice::getInstance().uploadGPUBlock();
        }
        friend class Divide::Renderer;
    };
};  // namespace Attorney
};  // namespace Divide

#include "GFXDevice.inl"

#endif
