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
#include "RenderPackage.h"
#include "GenericCommandPool.h"
#include "Core/Math/Headers/Line.h"
#include "Core/Math/Headers/MathMatrices.h"

#include "Platform/Video/Headers/RenderAPIWrapper.h"

#include "Rendering/Camera/Headers/Frustum.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

#include <stack>
#include <ArenaAllocator/arena_allocator.h>

namespace Divide {

enum class RenderStage : U32;
enum class RendererType : U32;
enum class SceneNodeType : U32;
enum class WindowEvent : U32;

class GUI;
class GUIText;

class GL_API;
class DX_API;

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
    class GFXDeviceAPI;
    class GFXDeviceGUI;
    class GFXDeviceKernel;
    class GFXDeviceRenderer;
    class GFXDeviceGraphicsResource;
};

/// Rough around the edges Adapter pattern abstracting the actual rendering API
/// and access to the GPU
DEFINE_SINGLETON(GFXDevice)
    friend class Attorney::GFXDeviceAPI;
    friend class Attorney::GFXDeviceGUI;
    friend class Attorney::GFXDeviceKernel;
    friend class Attorney::GFXDeviceRenderer;
    friend class Attorney::GFXDeviceGraphicsResource;

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
   // Enough to hold a cubemap (e.g. environment stage has 6 passes)
   static const U32 MAX_PASSES_PER_STAGE = 6;

   struct GPUBlock {
       GPUBlock() : _needsUpload(true),
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
               for (U8 i = 0; i < to_const_ubyte(Frustum::FrustPlane::COUNT); ++i) {
                   _frustumPlanes[i].set(0.0f);
               }
               for (U8 i = 0; i < to_const_ubyte(Frustum::FrustPlane::COUNT); ++i) {
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
           vec4<F32> _frustumPlanes[to_const_uint(Frustum::FrustPlane::COUNT)];
           vec4<F32> _clipPlanes[to_const_uint(Frustum::FrustPlane::COUNT)];

           inline F32 aspectRatio() const;
           inline vec2<F32> currentZPlanes() const;
           inline F32 FoV() const;
           inline F32 tanHFoV() const;

        } _data;

        mat4<F32> _viewMatrixInv;
        mat4<F32> _viewProjMatrixInv;

        bool _needsUpload = true;
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

    void debugDraw(const SceneRenderState& sceneRenderState, const Camera& activeCamera, RenderSubPassCmds& subPassesInOut);

    bool draw(const GenericDrawCommand& cmd);

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
    inline const mat4<F32>& getMatrix(const MATRIX& mode) const;
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

    inline void debugDrawFrustum(Frustum* frustum) { _debugFrustum = frustum; }

    inline RenderStage getRenderStage() const { return isPrePass() ? RenderStage::Z_PRE_PASS : _renderStage; }

    inline RenderStage getPrevRenderStage() const { return _prevRenderStage; }

    /// Return the last number of HIZ culled items
    U32 getLastCullCount() const;

    /// 2D rendering enabled
    inline bool is2DRendering() const { return _2DRendering; }

    /// returns the standard state block
    inline size_t getDefaultStateBlock(bool noDepth) const {
        return is2DRendering() ? _state2DRenderingHash
                               : (noDepth ? _defaultStateNoDepthHash
                                          : _defaultStateBlockHash);
    }

    inline RenderTarget& renderTarget(RenderTargetID target) {
        return _rtPool.renderTarget(target);
    }

    inline void renderTarget(RenderTargetID target, RenderTarget* newTarget) {
        _rtPool.set(target, newTarget);
    }

    inline RenderTargetHandle allocateRT(RenderTargetUsage targetUsage, const stringImpl& name) {
        return _rtPool.add(targetUsage, newRT(name));
    }

    inline RenderTargetHandle allocateRT(const stringImpl& name) {
        return allocateRT(RenderTargetUsage::OTHER, name);
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

    inline Arena::Statistics getObjectAllocStats() const { return _gpuObjectArena.statistics_; }

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

    inline void syncThreadToGPU(const std::thread::id& threadID, bool beginSync) {
        if (beginSync) {
            _api->syncToThread(threadID);
        }
    }

  protected:
    RenderTarget* newRT(const stringImpl& name) const;

    void drawDebugFrustum(RenderSubPassCmds& subPassesInOut);

    void setBaseViewport(const vec4<I32>& viewport);

    inline void drawText(const TextLabel& text,
                         const vec2<F32>& position,
                         size_t stateHash) {
        uploadGPUBlock();
        _api->drawText(text, position, stateHash);
    }

    void onChangeResolution(U16 w, U16 h);

  protected:
    friend class Camera;
    void renderFromCamera(Camera& camera);
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
    bool _viewportUpdate;
    vectorImpl<Line> _axisLines;
    IMPrimitive     *_axisGizmo;
    vectorImpl<Line> _axisLinesTransformed;

    Frustum         *_debugFrustum;
    IMPrimitive     *_debugFrustumPrimitive;

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

    std::atomic_int _graphicResources;
    /// Current viewport stack
    ViewportStack _viewport;

    GPUBlock _gpuBlock;

    DrawCommandList _drawCommandsCache;
    std::array<NodeData, Config::MAX_VISIBLE_NODES> _matricesData;
    std::array<U32, to_const_uint(RenderStage::COUNT) - 1> _lastCommandCount;
    std::array<U32, to_const_uint(RenderStage::COUNT) - 1> _lastNodeCount;

    mutable SharedLock _renderQueueLock;
    vectorImpl<RenderPackageQueue> _renderQueues;

    mutable SharedLock _GFXLoadQueueLock;
    std::deque<DELEGATE_CBK_PARAM<bool>> _GFXLoadQueue;

    ShaderBuffer* _gfxDataBuffer;
    GenericDrawCommand _defaultDrawCmd;

    GenericCommandPool  _commandPool;
    Time::ProfileTimer& _commandBuildTimer;

    mutable std::mutex _gpuObjectArenaMutex;
    mutable MyArena<Config::REQUIRED_RAM_SIZE / 4> _gpuObjectArena;

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

        static void syncThreadToGPU(const std::thread::id& threadID, bool beginSync) {
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

    class GFXDeviceGraphicsResource {
       private:
       static void onResourceCreate(GFXDevice& device) {
           device._graphicResources++;
       }
       static void onResourceDestroy(GFXDevice& device) {
           device._graphicResources--;
       }
       friend class Divide::GraphicsResource;
    };

    class GFXDeviceAPI {
        private:
        static void onRenderSubPass(GFXDevice& device) {
            device.uploadGPUBlock();
        }

        /// Get the entire list of clipping planes
        static const PlaneList& getClippingPlanes(GFXDevice& device) {
            return device._clippingPlanes;
        }

        friend class Divide::GL_API;
        friend class Divide::DX_API;
    };
};  // namespace Attorney
};  // namespace Divide

#include "GFXDevice.inl"

#endif
