/*
   Copyright (c) 2017 DIVIDE-Studio
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
#include "GFXShaderData.h"
#include "ScopedStates.h"
#include "RenderPackage.h"
#include "GenericCommandPool.h"
#include "Core/Math/Headers/Line.h"
#include "Core/Math/Headers/MathMatrices.h"

#include "Core/Headers/KernelComponent.h"

#include "Platform/Video/Headers/RenderStagePass.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"

#include "Rendering/Camera/Headers/Frustum.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

#include <stack>
#include <ArenaAllocator/arena_allocator.h>

class RenderDocManager;

namespace Divide {

enum class RendererType : U32;
enum class SceneNodeType : U32;
enum class WindowEvent : U32;

class GUI;
class GUIText;
class SceneGUIElements;

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
class GenericVertexData;
class ShaderComputeQueue;

struct ShaderBufferDescriptor;

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
    class KernelApplication;
};

namespace TypeUtil {
    const char* renderStageToString(RenderStage stage);
    const char* renderPassTypeToString(RenderPassType pass);
    RenderStage stringToRenderStage(const char* stage);
    RenderPassType stringToRenderPassType(const char* pass);
};

/// Rough around the edges Adapter pattern abstracting the actual rendering API
/// and access to the GPU
class GFXDevice : public KernelComponent {
    friend class Attorney::GFXDeviceAPI;
    friend class Attorney::GFXDeviceGUI;
    friend class Attorney::GFXDeviceKernel;
    friend class Attorney::GFXDeviceRenderer;
    friend class Attorney::GFXDeviceGraphicsResource;

protected:
    typedef std::stack<vec4<I32>> ViewportStack;

public:
    enum class ScreenTargets : U32 {
        ALBEDO = 0,
        NORMALS = 1,
        VELOCITY = 2,
        COUNT,
        ACCUMULATION = ALBEDO,
        REVEALAGE = NORMALS,
    };

    struct NodeData : private NonCopyable {
        mat4<F32> _worldMatrix;
        mat4<F32> _normalMatrixWV;
        mat4<F32> _colourMatrix;
        vec4<F32> _properties;

        NodeData()
        {
            _worldMatrix.identity();
            _normalMatrixWV.identity();
            _colourMatrix.zero();
        }

        void set(const NodeData& other);
    };

    struct DebugView {
        DebugView()
            : _textureBindSlot(0)
            , _sortIndex(-1)
        {
        }

        DebugView(U16 sortIndex)
            : _textureBindSlot(0)
            , _sortIndex(sortIndex)
        {
        }

        U8 _textureBindSlot;
        Texture_ptr _texture;
        ShaderProgram_ptr _shader;

        struct ShaderData {
            vectorImpl<std::pair<stringImpl, I32>>  _intValues;
            vectorImpl<std::pair<stringImpl, F32>>  _floatValues;
            vectorImpl<std::pair<stringImpl, bool>> _boolValues;
        } _shaderData;

        I16 _sortIndex;
    };
    FWD_DECLARE_MANAGED_CLASS(DebugView);

public:  // GPU interface
    explicit GFXDevice(Kernel& parent);
    ~GFXDevice();

    static const U32 MaxFrameQueueSize = 2;
    static_assert(MaxFrameQueueSize > 0, "FrameQueueSize is invalid!");

    ErrorCode initRenderingAPI(I32 argc, char** argv, const vec2<U16>& renderResolution);
    void closeRenderingAPI();

    inline void setAPI(RenderAPI API) { _API_ID = API; }
    inline RenderAPI getAPI() const { return _API_ID; }

    void idle();
    void beginFrame();
    void endFrame(bool swapBuffers);

    /// Set all of the needed API specific settings for 2D (Ortho) / 3D (Perspective) rendering
    /// Returns true if the state was changed or false if it was already set
    bool toggle2D(bool state);

    void debugDraw(const SceneRenderState& sceneRenderState, const Camera& activeCamera, RenderSubPassCmds& subPassesInOut);

    bool draw(const GenericDrawCommand& cmd);

    void lockQueue(RenderBinType type);
    void unlockQueue(RenderBinType type);
    U32  renderQueueSize(RenderBinType queueType);
    void addToRenderQueue(RenderBinType queueType, const RenderPackage& package);
    void renderQueueToSubPasses(RenderBinType queueType, RenderSubPassCmd& commandsInOut);
    void flushCommandBuffer(const CommandBuffer& commandBuffer);

    /// Sets the current render stage.
    ///@param stage Is used to inform the rendering pipeline what we are rendering.
    ///Shadows? reflections? etc
    inline bool isDepthStage() const;
    /// Clipping plane management. All the clipping planes are handled by shader programs only!
    void updateClipPlanes();
    /// disable or enable a clip plane by index
    inline void toggleClipPlane(ClipPlaneIndex index, const bool state);
    /// modify a single clip plane by index
    inline void setClipPlane(ClipPlaneIndex index, const Plane<F32>& p, bool state);
    /// set a new list of clipping planes. The old one is discarded
    inline void setClipPlanes(const ClipPlaneList& clipPlanes);
    /// clear all clipping planes
    inline void resetClipPlanes();

    /// Generate a cubemap from the given position
    /// It renders the entire scene graph (with culling) as default
    /// use the callback param to override the draw function
    void generateCubeMap(RenderTargetID cubeMap,
        const U16 arrayOffset,
        const vec3<F32>& pos,
        const vec2<F32>& zPlanes,
        const RenderStagePass& stagePass,
        U32 passIndex,
        Camera* camera = nullptr);

    void generateDualParaboloidMap(RenderTargetID targetBuffer,
        const U16 arrayOffset,
        const vec3<F32>& pos,
        const vec2<F32>& zPlanes,
        const RenderStagePass& stagePass,
        U32 passIndex,
        Camera* camera = nullptr);

    void getMatrix(const MATRIX& mode, mat4<F32>& mat) const;
    /// Alternative to the normal version of getMatrix
    inline const mat4<F32>& getMatrix(const MATRIX& mode) const;
    /// Access (Read Only) rendering data used by the GFX
    inline const GFXShaderData::GPUData& renderingData() const;
    /// Register a function to be called in the 2D rendering fase of the GFX Flush
    /// routine. Use callOrder for sorting purposes
    inline void add2DRenderFunction(const GUID_DELEGATE_CBK& callback, U32 callOrder);
    inline void remove2DRenderFunction(const GUID_DELEGATE_CBK& callback);
    /// Returns true if the viewport was changed
    bool restoreViewport();
    /// Returns true if the viewport was changed
    bool setViewport(const vec4<I32>& viewport);
    inline bool setViewport(I32 x, I32 y, I32 width, I32 height);

    void setSceneZPlanes(const vec2<F32>& zPlanes);

    /// Switch between fullscreen rendering
    void toggleFullScreen();
    void increaseResolution();
    void decreaseResolution();
    bool loadInContext(const CurrentContext& context, const DELEGATE_CBK<void, const Task&>& callback);

    /// Save a screenshot in TGA format
    void Screenshot(const stringImpl& filename);

    Renderer& getRenderer() const;
    void setRenderer(RendererType rendererType);

    ShaderComputeQueue& shaderComputeQueue();
    const ShaderComputeQueue& shaderComputeQueue() const;

    void resizeHistory(U8 historySize);
    void historyIndex(U8 index, bool copyPrevious);

public:  // Accessors and Mutators
    inline const GPUState& gpuState() const { return _state; }

    inline GPUState& gpuState() { return _state; }

    static void setFrameInterpolationFactor(const D64 interpolation) { s_interpolationFactor = interpolation; }
    static D64 getFrameInterpolationFactor() { return s_interpolationFactor; }

    inline void setGPUVendor(GPUVendor gpuvendor) { _GPUVendor = gpuvendor; }
    inline GPUVendor getGPUVendor() const { return _GPUVendor; }

    inline void setGPURenderer(GPURenderer gpurenderer) { _GPURenderer = gpurenderer; }
    inline GPURenderer getGPURenderer() const { return _GPURenderer; }

    inline void debugDrawFrustum(Frustum* frustum) { _debugFrustum = frustum; }

    inline const RenderStagePass& getRenderStage() const { return _renderStagePass; }

    inline const RenderStagePass&  getPrevRenderStage() const { return _prevRenderStagePass; }

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

    inline const Texture_ptr& getPrevDepthBuffer() const {
        return _prevDepthBuffers[_historyIndex];
    }

    inline RenderTarget& renderTarget(RenderTargetID target) {
        return _rtPool->renderTarget(target);
    }

    inline void renderTarget(RenderTargetID target, RenderTarget* newTarget) {
        _rtPool->set(target, newTarget);
    }

    inline RenderTargetHandle allocateRT(RenderTargetUsage targetUsage, const RenderTargetDescriptor& descriptor) {
        return _rtPool->add(targetUsage, newRT(descriptor));
    }

    inline RenderTargetHandle allocateRT(const RenderTargetDescriptor& descriptor) {
        return allocateRT(RenderTargetUsage::OTHER, descriptor);
    }

    inline bool deallocateRT(RenderTargetHandle& handle) {
        return _rtPool->remove(handle);
    }

    RenderDetailLevel shadowDetailLevel() const { return _shadowDetailLevel; }

    RenderDetailLevel renderDetailLevel() const { return _renderDetailLevel; }

    void shadowDetailLevel(RenderDetailLevel detailLevel) {
        _shadowDetailLevel = detailLevel;
    }

    void renderDetailLevel(RenderDetailLevel detailLevel) {
        _renderDetailLevel = detailLevel;
    }

    inline U32 getFrameCount() const { return FRAME_COUNT; }

    inline I32 getDrawCallCount() const { return FRAME_DRAW_CALLS_PREV; }

    inline Arena::Statistics getObjectAllocStats() const { return _gpuObjectArena.statistics_; }

    inline void registerDrawCall() { registerDrawCalls(1); }

    inline void registerDrawCalls(U32 count) { FRAME_DRAW_CALLS += count; }

    inline const vec4<I32>& getCurrentViewport() const { return _viewport.top(); }

    inline const RenderStagePass& setRenderStagePass(const RenderStagePass& stage);

    void addDebugView(const std::shared_ptr<DebugView>& view);

public:
    IMPrimitive*       newIMP() const;
    VertexBuffer*      newVB() const;
    PixelBuffer*       newPB(PBType type = PBType::PB_TEXTURE_2D) const;
    GenericVertexData* newGVD(const U32 ringBufferLength) const;
    Texture*           newTexture(size_t descriptorHash,
                                  const stringImpl& name,
                                  const stringImpl& resourceName,
                                  const stringImpl& resourceLocation,
                                  bool isFlipped,
                                  bool asyncLoad,
                                  const TextureDescriptor& texDescriptor) const;
    ShaderProgram*     newShaderProgram(size_t descriptorHash,
                                        const stringImpl& name,
                                        const stringImpl& resourceName,
                                        const stringImpl& resourceLocation,
                                        bool asyncLoad) const;
    ShaderBuffer*      newSB(const ShaderBufferDescriptor& descriptor) const;

    Pipeline           newPipeline(const PipelineDescriptor& descriptor) const;

public:  // Direct API calls
    inline U64 getFrameDurationGPU() {
        return _api->getFrameDurationGPU();
    }

    inline void pushDebugMessage(const char* message, I32 id) {
        if (Config::ENABLE_GPU_VALIDATION) {
            _api->pushDebugMessage(message, id);
        }
    }

    inline void popDebugMessage() {
        if (Config::ENABLE_GPU_VALIDATION) {
            _api->popDebugMessage();
        }
    }

protected:
    friend class GFXRTPool;
    RenderTarget* newRT(const RenderTargetDescriptor& descriptor) const;

    void drawDebugFrustum(RenderSubPassCmds& subPassesInOut);

    void setBaseViewport(const vec4<I32>& viewport);

    inline void drawText(const vectorImpl<GUITextBatchEntry>& batch) {
        uploadGPUBlock();
        _api->drawText(batch);
    }

    void onChangeResolution(U16 w, U16 h);

    void renderDebugViews();
    
protected:
    friend class Camera;
    void renderFromCamera(Camera& camera);
    void onCameraUpdate(const Camera& camera);
    void onCameraChange(const Camera& camera);

    void flushDisplay(const vec4<I32>& targetViewport);

protected:
    friend class SceneManager;
    friend class RenderPass;
    friend class RenderPassManager;

    void occlusionCull(const RenderPass::BufferData& bufferData, const Texture_ptr& depthBuffer);

    void buildDrawCommands(const RenderQueue::SortedQueues& sortedNodes,
                           SceneRenderState& sceneRenderState,
                           RenderPass::BufferData& bufferData,
                           bool refreshNodeData);

    bool batchCommands(GenericDrawCommands& commands) const;
    void constructHIZ(RenderTarget& depthBuffer);

    RenderAPIWrapper& getAPIImpl() { return *_api; }
    const RenderAPIWrapper& getAPIImpl() const { return *_api; }

private:
    void updateViewportInternal(const vec4<I32>& viewport);
    void updateViewportInternal(I32 x, I32 y, I32 width, I32 height);
    /// Upload draw related data to the GPU (view & projection matrices, viewport settings, etc)
    void uploadGPUBlock();

    ErrorCode createAPIInstance();

    NodeData& processVisibleNode(const SceneGraphNode& node, U32 dataIndex);

    mat4<F32>& getMatrixInternal(const MATRIX& mode);
    const mat4<F32>& getMatrixInternal(const MATRIX& mode) const;

private:
    std::unique_ptr<RenderAPIWrapper> _api;

    Renderer* _renderer;

    /// Pointer to a shader creation queue
    ShaderComputeQueue* _shaderComputeQueue;

    Camera* _cubeCamera;
    Camera* _2DCamera;
    Camera* _dualParaboloidCamera;

    RenderStagePass _renderStagePass;
    RenderStagePass _prevRenderStagePass;
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
    GFXRTPool* _rtPool;

    U8 _historyIndex;
    vectorImpl<Texture_ptr> _prevDepthBuffers;

    /*State management */
    bool _stateBlockByDescription;
    size_t _defaultStateBlockHash;
    /// The default render state buth with depth testing disabled
    size_t _defaultStateNoDepthHash;
    /// Special render state for 2D rendering
    size_t _state2DRenderingHash;
    size_t _stateDepthOnlyRenderingHash;
    /// The interpolation factor between the current and the last frame
    ClipPlaneList _clippingPlanes;

    bool _2DRendering;
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
    RenderDetailLevel _renderDetailLevel;

    typedef std::pair<I64, DELEGATE_CBK<void>> GUID2DCbk;
    mutable SharedLock _2DRenderQueueLock;
    vectorImpl<std::pair<U32, GUID2DCbk > > _2dRenderQueue;

    SharedLock _graphicsResourceMutex;
    vectorImpl<I64> _graphicResources;
    /// Current viewport stack
    ViewportStack _viewport;

    GFXShaderData _gpuBlock;

    DrawCommandList _drawCommandsCache;
    std::array<NodeData, Config::MAX_VISIBLE_NODES> _matricesData;
    std::array<U32, to_base(RenderStage::COUNT) - 1> _lastCommandCount;
    std::array<U32, to_base(RenderStage::COUNT) - 1> _lastNodeCount;

    vectorImpl<DebugView_ptr> _debugViews;

    std::array<RenderPackageQueue, to_base(RenderBinType::COUNT)> _renderQueues;

    mutable SharedLock _GFXLoadQueueLock;
    std::deque<DELEGATE_CBK<void, const Task&>> _GFXLoadQueue;

    ShaderBuffer* _gfxDataBuffer;
    GenericDrawCommand _defaultDrawCmd;

    GenericCommandPool  _commandPool;
    Time::ProfileTimer& _commandBuildTimer;

    std::shared_ptr<RenderDocManager> _renderDocManager;
    mutable std::mutex _gpuObjectArenaMutex;
    mutable MyArena<Config::REQUIRED_RAM_SIZE / 4> _gpuObjectArena;

    static D64 s_interpolationFactor;
};

namespace Attorney {
    class GFXDeviceGUI {
    private:
        static void drawText(GFXDevice& device, const vectorImpl<GUITextBatchEntry>& batch) {
            return device.drawText(batch);
        }

        friend class Divide::GUI;
        friend class Divide::GUIText;
        friend class Divide::SceneGUIElements;
    };

    class GFXDeviceKernel {
    private:
        static void onCameraUpdate(GFXDevice& device, const Camera& camera) {
            device.onCameraUpdate(camera);
        }

        static void onCameraChange(GFXDevice& device, const Camera& camera) {
            device.onCameraChange(camera);
        }

        static void flushDisplay(GFXDevice& device, const vec4<I32>& targetViewport) {
            device.flushDisplay(targetViewport);
        }

        static void onChangeWindowSize(GFXDevice& device, U16 w, U16 h) {
            device.setBaseViewport(vec4<I32>(0, 0, w, h));
        }

        static void onChangeRenderResolution(GFXDevice& device, U16 w, U16 h) {
            device.onChangeResolution(w, h);
        }

        friend class Divide::Kernel;
        friend class Divide::Attorney::KernelApplication;
    };

    class GFXDeviceRenderer {
        private:
        static void uploadGPUBlock(GFXDevice& device) {
            device.uploadGPUBlock();
        }
        friend class Divide::Renderer;
    };

    class GFXDeviceGraphicsResource {
       private:
       static void onResourceCreate(GFXDevice& device, I64 GUID) {
           WriteLock w_lock(device._graphicsResourceMutex);
           device._graphicResources.push_back(GUID);
       }
       static void onResourceDestroy(GFXDevice& device, I64 GUID) {
           WriteLock w_lock(device._graphicsResourceMutex);
           vectorImpl<I64>::iterator it;
           it = std::find_if(std::begin(device._graphicResources),
                std::end(device._graphicResources),
                [&GUID](I64 crtGUID) -> bool { return GUID == crtGUID; });
           assert(it != std::cend(device._graphicResources));
           device._graphicResources.erase(it);
   
       }
       friend class Divide::GraphicsResource;
    };

    class GFXDeviceAPI {
        private:
        static void onRenderSubPass(GFXDevice& device) {
            device.uploadGPUBlock();
        }

        /// Get the entire list of clipping planes
        static const ClipPlaneList& getClippingPlanes(GFXDevice& device) {
            return device._clippingPlanes;
        }

        friend class Divide::GL_API;
        friend class Divide::DX_API;
    };
};  // namespace Attorney
};  // namespace Divide

#include "GFXDevice.inl"

#endif
