/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _HARDWARE_VIDEO_GFX_DEVICE_H_
#define _HARDWARE_VIDEO_GFX_DEVICE_H_

#include "config.h"

#include "GFXState.h"
#include "GFXRTPool.h"
#include "GFXShaderData.h"
#include "CommandBufferPool.h"
#include "Core/Math/Headers/Line.h"
#include "Core/Math/Headers/MathMatrices.h"

#include "Core/Headers/KernelComponent.h"
#include "Core/Headers/PlatformContextComponent.h"

#include "Platform/Video/Headers/RenderStagePass.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"

#include "Rendering/Camera/Headers/Frustum.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

#include <stack>
#include <ArenaAllocator/arena_allocator.h>

class RenderDocManager;

namespace Divide {

enum class RendererType : U8;
enum class SceneNodeType : U16;
enum class WindowEvent : U8;

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

struct SizeChangeParams;
struct SizeChangeParams;
struct ShaderBufferDescriptor;

FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_STRUCT(DescriptorSet);

namespace Time {
    class ProfileTimer;
};

namespace Attorney {
    class GFXDeviceAPI;
    class GFXDeviceGUI;
    class GFXDeviceKernel;
    class GFXDeviceGraphicsResource;
    class GFXDeviceGFXRTPool;
    class KernelApplication;
};

namespace TypeUtil {
    const char* renderStageToString(RenderStage stage);
    const char* renderPassTypeToString(RenderPassType pass);
    RenderStage stringToRenderStage(const char* stage);
    RenderPassType stringToRenderPassType(const char* pass);
};

struct DebugView {
    DebugView()
        : _textureBindSlot(0)
        , _sortIndex(-1)
    {
    }

    DebugView(U16 sortIndex)
        : _textureBindSlot(0)
        , _sortIndex(to_I16(sortIndex))
    {
    }

    U8 _textureBindSlot;
    Texture_ptr _texture;
    ShaderProgram_ptr _shader;
    PushConstants _shaderData;

    I16 _sortIndex;
    stringImpl _name;
};

FWD_DECLARE_MANAGED_STRUCT(DebugView);

/// Rough around the edges Adapter pattern abstracting the actual rendering API
/// and access to the GPU
class GFXDevice : public KernelComponent, public PlatformContextComponent {
    friend class Attorney::GFXDeviceAPI;
    friend class Attorney::GFXDeviceGUI;
    friend class Attorney::GFXDeviceKernel;
    friend class Attorney::GFXDeviceGraphicsResource;
    friend class Attorney::GFXDeviceGFXRTPool;

public:
    enum class ScreenTargets : U8 {
        ALBEDO = 0,
        NORMALS = 1,
        VELOCITY = 2,
        COUNT,
        ACCUMULATION = ALBEDO,
        REVEALAGE = NORMALS,
        MODULATE = VELOCITY
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
    void endFrame();

    void debugDraw(const SceneRenderState& sceneRenderState, const Camera& activeCamera, GFX::CommandBuffer& bufferInOut);

    void flushCommandBuffer(GFX::CommandBuffer& commandBuffer);
    
    /// Generate a cubemap from the given position
    /// It renders the entire scene graph (with culling) as default
    /// use the callback param to override the draw function
    void generateCubeMap(
        RenderTargetID cubeMap,
        const U16 arrayOffset,
        const vec3<F32>& pos,
        const vec2<F32>& zPlanes,
        RenderStagePass stagePass,
        U32 passIndex,
        GFX::CommandBuffer& commandsInOut,
        Camera* camera = nullptr);

    void generateDualParaboloidMap(RenderTargetID targetBuffer,
        const U16 arrayOffset,
        const vec3<F32>& pos,
        const vec2<F32>& zPlanes,
        RenderStagePass stagePass,
        U32 passIndex,
        GFX::CommandBuffer& commandsInOut,
        Camera* camera = nullptr);

    /// Access (Read Only) rendering data used by the GFX
    inline const GFXShaderData::GPUData& renderingData() const;
    /// Returns true if the viewport was changed
    bool setViewport(const Rect<I32>& viewport);
    inline bool setViewport(I32 x, I32 y, I32 width, I32 height);
    inline const Rect<I32>& getViewport() const;

    inline F32 renderingAspectRatio() const;
    inline const vec2<U16>& renderingResolution() const;

    /// Switch between fullscreen rendering
    void toggleFullScreen();
    void increaseResolution();
    void decreaseResolution();

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

    inline void debugDrawFrustum(Frustum* frustum) { _debugFrustum = frustum; }

    /// returns the standard state block
    inline size_t getDefaultStateBlock(bool noDepth) const {
        return noDepth ? _defaultStateNoDepthHash : _defaultStateBlockHash;
    }

    inline size_t get2DStateBlock() const {
        return _state2DRenderingHash;
    }

    inline const Texture_ptr& getPrevDepthBuffer() const {
        return _prevDepthBuffers[_historyIndex];
    }

    inline GFXRTPool& renderTargetPool() {
        return *_rtPool;
    }

    inline const GFXRTPool& renderTargetPool() const {
        return *_rtPool;
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
    /// Return the last number of HIZ culled items
    inline U32 getLastCullCount() const { return LAST_CULL_COUNT; }

    inline Arena::Statistics getObjectAllocStats() const { return _gpuObjectArena.statistics_; }

    inline void registerDrawCall() { registerDrawCalls(1); }

    inline void registerDrawCalls(U32 count) { FRAME_DRAW_CALLS += count; }

    inline const Rect<I32>& getCurrentViewport() const { return _viewport; }

    inline const Rect<I32>& getBaseViewport() const { return _baseViewport; }

    DebugView* addDebugView(const std::shared_ptr<DebugView>& view);

    static void setFrameInterpolationFactor(const D64 interpolation) { s_interpolationFactor = interpolation; }
    static D64 getFrameInterpolationFactor() { return s_interpolationFactor; }
    static void setGPUVendor(GPUVendor gpuvendor) { s_GPUVendor = gpuvendor; }
    static GPUVendor getGPUVendor() { return s_GPUVendor; }
    static void setGPURenderer(GPURenderer gpurenderer) { s_GPURenderer = gpurenderer; }
    static GPURenderer getGPURenderer() { return s_GPURenderer; }

public:
    IMPrimitive*       newIMP() const;
    VertexBuffer*      newVB() const;
    PixelBuffer*       newPB(PBType type = PBType::PB_TEXTURE_2D, const char* name = nullptr) const;
    GenericVertexData* newGVD(const U32 ringBufferLength, const char* name = nullptr) const;
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

    Pipeline*          newPipeline(const PipelineDescriptor& descriptor) const;
    DescriptorSet_ptr  newDescriptorSet() const;

    // Shortcuts
    void drawText(const TextElementBatch& batch, GFX::CommandBuffer& bufferInOut) const;

    // Render the texture over the full window dimensions regardless of the actual active rendering viewport 
    void drawTextureInRenderWindow(TextureData data, GFX::CommandBuffer& bufferInOut) const;
    // Render the texture using the full rendering viewport
    void drawTextureInRenderViewport(TextureData data, GFX::CommandBuffer& bufferInOut) const;
    // Render the texture using a custom viewport
    void drawTextureInViewport(TextureData data, const Rect<I32>& viewport, GFX::CommandBuffer& bufferInOut) const;

public:  // Direct API calls
    inline U32 getFrameDurationGPU() {
        return _api->getFrameDurationGPU();
    }

    inline vec2<U16> getDrawableSize(const DisplayWindow& window) const {
        return _api->getDrawableSize(window);
    }

    inline U32 getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const {
        return _api->getHandleFromCEGUITexture(textureIn);
    }

protected:
    RenderTarget* newRT(const RenderTargetDescriptor& descriptor) const;

    void drawDebugFrustum(GFX::CommandBuffer& bufferInOut);

    void setBaseViewport(const Rect<I32>& viewport);

    void drawText(const TextElementBatch& batch);

    void fitViewportInWindow(U16 w, U16 h);

    void onSizeChange(const SizeChangeParams& params);

    void renderDebugViews(GFX::CommandBuffer& bufferInOut);
    
    void stepResolution(bool increment);

protected:
    void blitToScreen(const Rect<I32>& targetViewport);

    void blitToRenderTarget(RenderTargetID targetID, const Rect<I32>& targetViewport);
    void blitToBuffer(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut);

protected:
    friend class SceneManager;
    friend class RenderPass;
    friend class RenderPassManager;

    void occlusionCull(const RenderPass::BufferData& bufferData,
                       U32 bufferIndex,
                       const Texture_ptr& depthBuffer,
                       const vec2<F32>& zPlanes,
                       GFX::CommandBuffer& bufferInOut) const;

    void constructHIZ(RenderTargetID depthBuffer, GFX::CommandBuffer& cmdBufferInOut) const;

    void updateCullCount(GFX::CommandBuffer& cmdBufferInOut);

    RenderAPIWrapper& getAPIImpl() { return *_api; }
    const RenderAPIWrapper& getAPIImpl() const { return *_api; }

private:
    /// Upload draw related data to the GPU (view & projection matrices, viewport settings, etc)
    void uploadGPUBlock();
    void setClipPlanes(const FrustumClipPlanes& clipPlanes);
    void renderFromCamera(const CameraSnapshot& cameraSnapshot);

    ErrorCode createAPIInstance();

private:
    std::unique_ptr<RenderAPIWrapper> _api;

    Renderer* _renderer;

    /// Pointer to a shader creation queue
    ShaderComputeQueue* _shaderComputeQueue;

    vector<Line> _axisLines;
    IMPrimitive     *_axisGizmo;
    vector<Line> _axisLinesTransformed;

    Frustum         *_debugFrustum;
    IMPrimitive     *_debugFrustumPrimitive;
    CameraSnapshot  _activeCameraSnapshot;
protected:
    RenderAPI _API_ID;
    GPUState _state;
    /* Rendering buffers.*/
    GFXRTPool* _rtPool;

    std::pair<vec2<U16>, bool> _resolutionChangeQueued;

    U8 _historyIndex;
    vector<Texture_ptr> _prevDepthBuffers;

    /*State management */
    bool _stateBlockByDescription;
    size_t _defaultStateBlockHash;
    /// The default render state buth with depth testing disabled
    size_t _defaultStateNoDepthHash;
    /// Special render state for 2D rendering
    size_t _state2DRenderingHash;
    size_t _stateDepthOnlyRenderingHash;
    /// The interpolation factor between the current and the last frame
    FrustumClipPlanes _clippingPlanes;

    bool _2DRendering;
    // number of draw calls (rough estimate)
    I32 FRAME_DRAW_CALLS;
    U32 FRAME_DRAW_CALLS_PREV;
    U32 FRAME_COUNT;

    U32 LAST_CULL_COUNT = 0;

    /// shader used to preview the depth buffer
    ShaderProgram_ptr _previewDepthMapShader;
    ShaderProgram_ptr _renderTargetDraw;
    ShaderProgram_ptr _HIZConstructProgram;
    ShaderProgram_ptr _HIZCullProgram;
    ShaderProgram_ptr _displayShader;
    ShaderProgram_ptr _textRenderShader;
    PushConstants _textRenderConstants;
    Pipeline* _textRenderPipeline = nullptr;

    /// Quality settings
    RenderDetailLevel _shadowDetailLevel;
    RenderDetailLevel _renderDetailLevel;
    
    SharedLock _graphicsResourceMutex;
    vector<std::pair<GraphicsResource::Type, I64>> _graphicResources;

    /// Current viewport stack
    Rect<I32> _viewport;
    Rect<I32> _baseViewport;

    vec2<U16> _renderingResolution;

    GFXShaderData _gpuBlock;

    std::array<U32, to_base(RenderStage::COUNT) - 1> _lastCommandCount;
    std::array<U32, to_base(RenderStage::COUNT) - 1> _lastNodeCount;

    vector<DebugView_ptr> _debugViews;
    
    ShaderBuffer* _gfxDataBuffer;
    GenericDrawCommand _defaultDrawCmd;

    MemoryPool<GenericDrawCommand> _commandPool;

    mutable SharedLock _descriptorSetPoolLock;
    mutable DescriptorSetPool _descriptorSetPool;
    
    mutable SharedLock _pipelineCacheLock;
    mutable hashMap<size_t, Pipeline> _pipelineCache;
    std::shared_ptr<RenderDocManager> _renderDocManager;
    mutable std::mutex _gpuObjectArenaMutex;
    mutable MyArena<Config::REQUIRED_RAM_SIZE / 4> _gpuObjectArena;

    static D64 s_interpolationFactor;
    static GPUVendor s_GPUVendor;
    static GPURenderer s_GPURenderer;
};

namespace Attorney {
    class GFXDeviceGUI {
    private:
        static void drawText(GFXDevice& device, const TextElementBatch& batch, GFX::CommandBuffer& bufferInOut) {
            return device.drawText(batch, bufferInOut);
        }
        friend class Divide::GUI;
        friend class Divide::GUIText;
        friend class Divide::SceneGUIElements;
    };

    class GFXDeviceKernel {
    private:
        static void blitToScreen(GFXDevice& device, const Rect<I32>& targetViewport) {
            device.blitToScreen(targetViewport);
        }

        static void blitToRenderTarget(GFXDevice& device, RenderTargetID targetID, const Rect<I32>& targetViewport) {
            device.blitToRenderTarget(targetID, targetViewport);
        }

        static void onSizeChange(GFXDevice& device, const SizeChangeParams& params) {
            device.onSizeChange(params);
        }
        
        friend class Divide::Kernel;
        friend class Divide::Attorney::KernelApplication;
    };

    class GFXDeviceGraphicsResource {
       private:
       static void onResourceCreate(GFXDevice& device, GraphicsResource::Type type, I64 GUID) {
           WriteLock w_lock(device._graphicsResourceMutex);
           device._graphicResources.emplace_back(type, GUID);
       }

       static void onResourceDestroy(GFXDevice& device, GraphicsResource::Type type, I64 GUID) {
           WriteLock w_lock(device._graphicsResourceMutex);
           vector<std::pair<GraphicsResource::Type, I64>>::iterator it;
           it = std::find_if(std::begin(device._graphicResources),
                std::end(device._graphicResources),
                [type, GUID](const std::pair<GraphicsResource::Type, I64> crtEntry) -> bool {
                    if (crtEntry.second == GUID) {
                        assert(crtEntry.first == type);
                        return true;
                    }
                    return false;
                });
           assert(it != std::cend(device._graphicResources));
           device._graphicResources.erase(it);
   
       }
       friend class Divide::GraphicsResource;
    };

    class GFXDeviceGFXRTPool {
        private:
        static RenderTarget* newRT(GFXDevice& device, const RenderTargetDescriptor& descriptor) {
            return device.newRT(descriptor);
        };

        friend class Divide::GFXRTPool;
    };
};  // namespace Attorney
};  // namespace Divide

#include "GFXDevice.inl"

#endif
