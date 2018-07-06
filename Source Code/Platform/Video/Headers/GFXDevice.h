/*
   Copyright (c) 2016 DIVIDE-Studio
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

#include "config.h"

#include "GFXState.h"
#include "GFXRTPool.h"
#include "ScopedStates.h"
#include "GenericCommandPool.h"

#include "Core/Math/Headers/Line.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Platform/Video/Headers/RenderAPIWrapper.h"

#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

#include <stack>

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
class Texture;
class Object3D;
class Renderer;
class IMPrimitive;
class PixelBuffer;
class VertexBuffer;
class SceneGraphNode;
class SceneRenderState;
FWD_DECLARE_MANAGED_CLASS(Texture);

namespace Time {
    class ProfileTimer;
};

namespace Attorney {
    class GFXDeviceGUI;
    class GFXDeviceKernel;
    class GFXDeviceRenderer;
};

/// Rough around the edges Adapter pattern abstracting the actual rendering API
/// and access to the GPU
DEFINE_SINGLETON(GFXDevice)
    friend class Attorney::GFXDeviceGUI;
    friend class Attorney::GFXDeviceKernel;
    friend class Attorney::GFXDeviceRenderer;
  protected:
    typedef std::stack<vec4<I32>> ViewportStack;

  public:
    struct NodeData {
        mat4<F32> _worldMatrix;
        mat4<F32> _normalMatrixWV;
        mat4<F32> _colourMatrix;
        vec4<F32> _properties;
        vec4<F32> _extraProperties;

        NodeData()
        {
            _worldMatrix.identity();
            _normalMatrixWV.identity();
            _colourMatrix.zero();
        }
        void set(const NodeData& other);
    };

  public:  // GPU specific data
   struct ShaderBufferBinding {
       ShaderBufferLocation _slot;
       ShaderBuffer* _buffer;
       vec2<U32>    _range;

       ShaderBufferBinding() 
            : ShaderBufferBinding(ShaderBufferLocation::COUNT,
                                  nullptr,
                                  vec2<U32>(0,0))
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
       RenderPackage() : _isRenderable(false),
                         _isOcclusionCullable(true)
       {
       }

       bool isCompatible(const RenderPackage& other) const;

       void clear();
       void set(const RenderPackage& other);

       inline bool isRenderable() const {
           return  _isRenderable;
       }

       inline bool isOcclusionCullable() const {
           return  _isOcclusionCullable;
       }

       inline void isRenderable(bool state) {
           _isRenderable = state;
       }

       inline void isOcclusionCullable(bool state) {
           _isOcclusionCullable = state;
       }

       ShaderBufferList _shaderBuffers;
       TextureDataContainer _textureData;
       GenericDrawCommands _drawCommands;

       private:
           bool _isRenderable;
           bool _isOcclusionCullable;
   };

   struct RenderQueue {
    public:
       RenderQueue() : _locked(false),
                       _currentCount(0)
       {
       }

       void clear();
       U32 size() const;
       bool empty() const;
       bool locked() const;
       const RenderPackage& getPackage(U32 idx) const;

    protected:
        friend class GFXDevice;
        RenderPackage& getPackage(U32 idx);
        RenderPackage& back();
        bool push_back(const RenderPackage& package);
        void reserve(U16 size);
        void lock();
        void unlock();

    protected:
       bool _locked;
       U32 _currentCount;
       vectorImpl<RenderPackage> _packages;
   };

   // Enough to hold a cubemap (e.g. environment stage has 6 passes)
   static const U32 MAX_PASSES_PER_STAGE = 6;

   struct GPUBlock {
       GPUBlock() : _updated(true),
                    _frustumDirty(true),
                    _data(GPUData())
       {
       }

       struct GPUData {
           GPUData()
           {
               _ProjectionMatrix.identity();
               _InvProjectionMatrix.identity();
               _ViewMatrix.identity();
               _ViewProjectionMatrix.identity();
               _cameraPosition.set(0.0f);
               _ViewPort.set(1.0f);
               _ZPlanesCombined.set(1.0f, 1.1f, 1.0f, 1.1f);
               _invScreenDimension.set(1.0f);
               _renderProperties.set(0.0f);
               for (U8 i = 0; i < 6; ++i) {
                   _frustumPlanes[i].set(0.0f);
               }
               for (U8 i = 0; i < Config::MAX_CLIP_PLANES; ++i) {
                   _frustumPlanes[i].set(1.0f);
               }
           }

           mat4<F32> _ProjectionMatrix;
           mat4<F32> _InvProjectionMatrix;
           mat4<F32> _ViewMatrix;
           mat4<F32> _ViewProjectionMatrix;
           vec4<F32> _cameraPosition; // xyz - position, w - aspect ratio
           vec4<F32> _ViewPort;
           vec4<F32> _ZPlanesCombined;  // xy - current, zw - main scene
           vec4<F32> _invScreenDimension; //xy - dims, zw - reserved;
           vec4<F32> _renderProperties;
           vec4<F32> _frustumPlanes[6];
           vec4<F32> _clipPlanes[Config::MAX_CLIP_PLANES];

           inline F32 aspectRatio() const;
           inline vec2<F32> currentZPlanes() const;
           inline F32 FoV() const;
           inline F32 tanHFoV() const;

        } _data;

        inline void update(bool viewMatrixUpdate) {
            mat4<F32>::Multiply(_data._ViewMatrix, _data._ProjectionMatrix, _data._ViewProjectionMatrix);
            if (viewMatrixUpdate) {
                _data._ViewMatrix.getInverse(_viewMatrixInv);
            }
            if (!viewMatrixUpdate) {
                _data._ProjectionMatrix.getInverse(_data._InvProjectionMatrix);
            }
            _data._ViewProjectionMatrix.getInverse(_viewProjMatrixInv);
            updateFrustumPlanes();
            _updated = true;
        }

        inline void updateFrustumPlanes();

        inline mat4<F32>& viewMatrixInv() { return _viewMatrixInv; }
        inline mat4<F32>& viewProjectionMatrixInv() { return _viewProjMatrixInv; }

        inline const mat4<F32>& viewMatrixInv() const { return _viewMatrixInv; }
        inline const mat4<F32>& viewProjectionMatrixInv() const { return _viewProjMatrixInv; }

        bool _updated;
       private:
           bool _frustumDirty;
           mat4<F32> _viewMatrixInv;
           mat4<F32> _viewProjMatrixInv;
   };

  public:  // GPU interface
    static const U32 MaxFrameQueueSize = 2;
    static_assert(MaxFrameQueueSize > 0, "FrameQueueSize is invalid!");

    ErrorCode initRenderingAPI(I32 argc, char** argv, const vec2<U16>& renderResolution);
    void closeRenderingAPI();

    inline void setAPI(RenderAPI API) { _API_ID = API; }
    inline RenderAPI getAPI() const { return _API_ID; }

    void idle();
    void beginFrame();
    void endFrame(bool swapBuffers);

    /// Set all of the needed API specific settings for 2D (Ortho) / 3D
    /// (Perspective) rendering
    void toggle2D(bool state);

    /**
     *@brief Returns an immediate mode emulation buffer that can be used to
     *construct geometry in a vertex by vertex manner.
     *@param allowPrimitiveRecycle If false, do not reuse old primitives and do not
     *delete it after x-frames.
     *(Don't use the primitive zombie feature)
     */
    IMPrimitive* getOrCreatePrimitive(bool allowPrimitiveRecycle = true);
    void debugDraw(const SceneRenderState& sceneRenderState);

    void draw(const GenericDrawCommand& cmd);

    void addToRenderQueue(U32 queueIndex, const RenderPackage& package);
    void renderQueueToSubPasses(RenderPassCmd& commandsInOut);
    void flushCommandBuffer(const CommandBuffer& commandBuffer);

    I32  reserveRenderQueue();

    /// Sets the current render stage.
    ///@param stage Is used to inform the rendering pipeline what we are rendering.
    ///Shadows? reflections? etc
    inline bool isDepthStage() const;
    inline bool isPrePass() const;
    inline void setPrePass(const bool state);
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

    /// Generate a cubemap from the given position
    /// It renders the entire scene graph (with culling) as default
    /// use the callback param to override the draw function
    void generateCubeMap(RenderTarget& cubeMap,
                         const U32 arrayOffset,
                         const vec3<F32>& pos,
                         const vec2<F32>& zPlanes,
                         RenderStage renderStage,
                         U32 passIndex);

    void generateDualParaboloidMap(RenderTarget& targetBuffer,
                                   const U32 arrayOffset,
                                   const vec3<F32>& pos,
                                   const vec2<F32>& zPlanes,
                                   RenderStage renderStage,
                                   U32 passIndex);
                                     
    void getMatrix(const MATRIX& mode, mat4<F32>& mat) const;
    /// Alternative to the normal version of getMatrix
    inline mat4<F32> getMatrix(const MATRIX& mode) const;
    /// Access (Read Only) rendering data used by the GFX
    inline const GPUBlock::GPUData& renderingData() const;
    /// Register a function to be called in the 2D rendering fase of the GFX Flush
    /// routine. Use callOrder for sorting purposes
    inline void add2DRenderFunction(const GUID_DELEGATE_CBK& callback, U32 callOrder);
    inline void remove2DRenderFunction(const GUID_DELEGATE_CBK& callback);
    void restoreViewport();
    void setViewport(const vec4<I32>& viewport);
    inline void setViewport(I32 x, I32 y, I32 width, I32 height);
    /// Switch between fullscreen rendering
    void toggleFullScreen();
    void increaseResolution();
    void decreaseResolution();
    bool loadInContext(const CurrentContext& context,
                       const DELEGATE_CBK_PARAM<bool>& callback);

    /// Save a screenshot in TGA format
    void Screenshot(const stringImpl& filename);

  public:  // Accessors and Mutators
    inline const GPUState& gpuState() const { return _state; }

    inline GPUState& gpuState() { return _state; }

    inline void setInterpolation(const D64 interpolation) {
        _interpolationFactor = interpolation;
    }

    inline D64 getInterpolation() const { return _interpolationFactor; }

    inline void setGPUVendor(GPUVendor gpuvendor) { _GPUVendor = gpuvendor; }
    inline GPUVendor getGPUVendor() const { return _GPUVendor; }

    inline void setGPURenderer(GPURenderer gpurenderer) { _GPURenderer = gpurenderer; }
    inline GPURenderer getGPURenderer() const { return _GPURenderer; }

    inline void drawDebugAxis(const bool state) { _drawDebugAxis = state; }

    inline bool drawDebugAxis() const { return _drawDebugAxis; }

    inline RenderStage getRenderStage() const { return isPrePass() ? RenderStage::Z_PRE_PASS : _renderStage; }

    inline RenderStage getPrevRenderStage() const { return _prevRenderStage; }

    /// Get the entire list of clipping planes
    inline const PlaneList& getClippingPlanes() const { return _clippingPlanes; }

    /// Return the last number of HIZ culled items
    U32 getLastCullCount() const;

    /// 2D rendering enabled
    inline bool is2DRendering() const { return _2DRendering; }

    /// returns the standard state block
    inline size_t getDefaultStateBlock(bool noDepth) const {
        return (noDepth ? _defaultStateNoDepthHash : _defaultStateBlockHash);
    }

    inline RenderTarget& renderTarget(RenderTargetID target) {
        return _rtPool.renderTarget(target);
    }

    inline RenderTarget& prevRenderTarget(RenderTargetID target) {
        return _rtPool.renderTarget(target);
    }

    inline void renderTarget(RenderTargetID target, RenderTarget* newTarget) {
        _rtPool.set(target, newTarget);
    }

    inline RenderTargetHandle allocateRT(RenderTargetUsage targetUsage) {
        return _rtPool.add(targetUsage, newRT());
    }

    inline RenderTargetHandle allocateRT() {
        return allocateRT(RenderTargetUsage::OTHER);
    }

    inline bool deallocateRT(RenderTargetHandle& handle) {
        return _rtPool.remove(handle);
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

  public:
      IMPrimitive*       newIMP() const;
      VertexBuffer*      newVB() const;
      PixelBuffer*       newPB(PBType type = PBType::PB_TEXTURE_2D) const;
      GenericVertexData* newGVD(const U32 ringBufferLength) const;
      Texture*           newTexture(const stringImpl& name,
                                    const stringImpl& resourceLocation,
                                    TextureType type,
                                    bool asyncLoad) const;
      ShaderProgram*     newShaderProgram(const stringImpl& name,
                                          const stringImpl& resourceLocation,
                                          bool asyncLoad) const;
      ShaderBuffer*      newSB(const U32 ringBufferLength = 1,
                               const bool unbound = false,
                               const bool persistentMapped = true,
                               BufferUpdateFrequency frequency =
                                     BufferUpdateFrequency::ONCE) const;
  public:  // Direct API calls


    inline U64 getFrameDurationGPU() {
        return _api->getFrameDurationGPU();
    }

    inline void syncThreadToGPU(std::thread::id threadID, bool beginSync) {
        if (beginSync) {
            _api->syncToThread(threadID);
        }
    }

  protected:
    RenderTarget* newRT() const;

    void setBaseViewport(const vec4<I32>& viewport);

    inline void drawText(const TextLabel& text,
                         const vec2<F32>& position,
                         size_t stateHash) {
        uploadGPUBlock();
        _api->drawText(text, position, stateHash);
    }

    void drawDebugAxis(const SceneRenderState& sceneRenderState);

    void onChangeResolution(U16 w, U16 h);

    static void computeFrustumPlanes(const mat4<F32>& invViewProj, vec4<F32>* planesOut);
  protected:
    friend class Camera;

    F32* lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& eyePos);
    /// sets an ortho projection, updating any listeners if needed
    F32* setProjection(const vec4<F32>& rect, const vec2<F32>& planes);
    /// sets a perspective projection, updating any listeners if needed
    F32* setProjection(F32 FoV, F32 aspectRatio, const vec2<F32>& planes);

    void onCameraUpdate(Camera& camera);
    void onCameraChange(Camera& camera);

    void flushDisplay();

   protected:
    friend class SceneManager;
    friend class RenderPass;
    friend class RenderPassManager;

    void occlusionCull(const RenderPass::BufferData& bufferData,
                       const Texture_ptr& depthBuffer);
    void buildDrawCommands(RenderPassCuller::VisibleNodeList& visibleNodes,
                           SceneRenderState& sceneRenderState,
                           RenderPass::BufferData& bufferData,
                           bool refreshNodeData);
    bool batchCommands(GenericDrawCommand& previousIDC,
                       GenericDrawCommand& currentIDC) const;
    void constructHIZ(RenderTarget& depthBuffer);

    RenderAPIWrapper& getAPIImpl() { return *_api; }
    const RenderAPIWrapper& getAPIImpl() const { return *_api; }

   private:
    GFXDevice();
    ~GFXDevice();

    void previewDepthBuffer();
    void updateViewportInternal(const vec4<I32>& viewport);
    void updateViewportInternal(I32 x, I32 y, I32 width, I32 height);
    /// Upload draw related data to the GPU (view & projection matrices, viewport settings, etc)
    void uploadGPUBlock();

    ErrorCode createAPIInstance();

    NodeData& processVisibleNode(const SceneGraphNode& node, U32 dataIndex);

    RenderTarget& activeRenderTarget();
    const RenderTarget& activeRenderTarget() const;

    mat4<F32>& getMatrixInternal(const MATRIX& mode);
    const mat4<F32>& getMatrixInternal(const MATRIX& mode) const;

  private:
    Camera* _cubeCamera;
    Camera* _2DCamera;
    Camera* _dualParaboloidCamera;

    RenderTarget* _activeRenderTarget;
    RenderAPIWrapper* _api;
    RenderStage _renderStage;
    RenderStage _prevRenderStage;
    bool _drawDebugAxis;
    bool _viewportUpdate;
    vectorImpl<Line> _axisLines;
    IMPrimitive     *_axisGizmo;
    vectorImpl<Line> _axisLinesTransformed;

    RenderTargetHandle _previousDepthBuffer;
  protected:
    RenderAPI _API_ID;
    GPUVendor _GPUVendor;
    GPURenderer _GPURenderer;
    GPUState _state;
    /* Rendering buffers.*/
    GFXRTPool _rtPool;
    /*State management */
    bool _stateBlockByDescription;
    size_t _defaultStateBlockHash;
    /// The default render state buth with depth testing disabled
    size_t _defaultStateNoDepthHash;
    /// Special render state for 2D rendering
    size_t _state2DRenderingHash;
    size_t _stateDepthOnlyRenderingHash;
    /// The interpolation factor between the current and the last frame
    D64 _interpolationFactor;
    PlaneList _clippingPlanes;
    bool _2DRendering;
    bool _isPrePassStage;
    // number of draw calls (rough estimate)
    I32 FRAME_DRAW_CALLS;
    U32 FRAME_DRAW_CALLS_PREV;
    U32 FRAME_COUNT;
    /// shader used to preview the depth buffer
    ShaderProgram_ptr _previewDepthMapShader;
    ShaderProgram_ptr _renderTargetDraw;
    ShaderProgram_ptr _HIZConstructProgram;
    ShaderProgram_ptr _HIZCullProgram;
    ShaderProgram_ptr _displayShader;

    /// Quality settings
    RenderDetailLevel _shadowDetailLevel;

    typedef std::pair<I64, DELEGATE_CBK<>> GUID2DCbk;
    mutable SharedLock _2DRenderQueueLock;
    vectorImpl<std::pair<U32, GUID2DCbk > > _2dRenderQueue;

    /// Immediate mode emulation shader
    ShaderProgram_ptr _imShader;
    I32 _imShaderTextureFlag;
    I32 _imShaderWorldMatrix;
    /// The interface that coverts IM calls to VB data
    SharedLock _imInterfaceLock;
    vectorImpl<IMPrimitive*> _imInterfaces;
    vectorImpl<IMPrimitive*> _activeImInterfaces;
    /// Current viewport stack
    ViewportStack _viewport;

    GPUBlock _gpuBlock;

    DrawCommandList _drawCommandsCache;
    std::array<NodeData, Config::MAX_VISIBLE_NODES> _matricesData;
    std::array<U32, to_const_uint(RenderStage::COUNT) - 1> _lastCommandCount;
    std::array<U32, to_const_uint(RenderStage::COUNT) - 1> _lastNodeCount;

    mutable SharedLock _renderQueueLock;
    vectorImpl<RenderQueue> _renderQueues;

    mutable SharedLock _GFXLoadQueueLock;
    std::deque<DELEGATE_CBK_PARAM<bool>> _GFXLoadQueue;

    ShaderBuffer* _gfxDataBuffer;
    GenericDrawCommand _defaultDrawCmd;

    GenericCommandPool _commandPool;
    Time::ProfileTimer& _commandBuildTimer;

END_SINGLETON

namespace Attorney {
    class GFXDeviceGUI {
    private:
        static void drawText(const TextLabel& text,
                             size_t stateHash,
                             const vec2<F32>& position) {
            return GFXDevice::instance().drawText(text, position, stateHash);
        }

        friend class Divide::GUI;
        friend class Divide::GUIText;
    };

    class GFXDeviceKernel {
    private:
        static void onCameraUpdate(Camera& camera) {
            GFXDevice::instance().onCameraUpdate(camera);
        }

        static void onCameraChange(Camera& camera) {
            GFXDevice::instance().onCameraChange(camera);
        }

        static void flushDisplay() {
            GFXDevice::instance().flushDisplay();
        }

        static void onChangeWindowSize(U16 w, U16 h) {
            GFXDevice::instance().setBaseViewport(vec4<I32>(0, 0, w, h));
        }

        static void onChangeRenderResolution(U16 w, U16 h) {
            GFXDevice::instance().onChangeResolution(w, h);
        }

        static void syncThreadToGPU(std::thread::id threadID, bool beginSync) {
            if (beginSync) {
                GFXDevice::instance().syncThreadToGPU(threadID, beginSync);
            }
        }

        friend class Divide::Kernel;
    };

    class GFXDeviceRenderer {
        private:
        static void uploadGPUBlock() {
            GFXDevice::instance().uploadGPUBlock();
        }
        friend class Divide::Renderer;
    };
};  // namespace Attorney
};  // namespace Divide

#include "GFXDevice.inl"

#endif
