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

#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Direct3D/Headers/DXWrapper.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"
#include "Managers/Headers/RenderPassManager.h"

#include <stack>
#include <future>

namespace Divide {

enum class RenderStage : U32;
enum class RendererType : U32;
enum class SceneNodeType : U32;

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

/// Rough around the edges Adapter pattern abstracting the actual rendering API
/// and access to the GPU
DEFINE_SINGLETON_EXT1_W_SPECIFIER(GFXDevice, RenderAPIWrapper, final)
  protected:
    typedef hashMapImpl<size_t, RenderStateBlock*> RenderStateMap;
    typedef std::stack<vec4<I32>, vectorImpl<vec4<I32> > > ViewportStack;

    struct NodeData {
        mat4<F32> _matrix[4];

        NodeData() {
            _matrix[0].identity();
            _matrix[1].identity();
            _matrix[2].zero();
            _matrix[3].zero();
        }

        inline void set(const NodeData& other) {
            for (U8 i = 0; i < 4; ++i) {
                _matrix[i].set(other._matrix[i]);
            }
        }
    };

    enum class GPUBuffer : U32 {
        NODE_BUFFER = 0,
        GPU_BUFFER = 1,
        CMD_BUFFER = 2,
        COUNT
    };
  public:  // GPU specific data
   struct ShaderBufferBinding {
       ShaderBufferLocation _slot;
       ShaderBuffer* _buffer;
       vec2<ptrdiff_t> _range;

       ShaderBufferBinding() : _buffer(nullptr)
       {
       }

       ShaderBufferBinding(ShaderBufferLocation slot,
                           ShaderBuffer* buffer,
                           const vec2<ptrdiff_t>& range)
           : _slot(slot), _buffer(buffer), _range(range)
       {
       }

       void set(ShaderBufferLocation slot,
                ShaderBuffer* buffer,
                const vec2<ptrdiff_t>& range) {
           _slot = slot;
           _buffer = buffer;
           _range.set(range);
       }
   };

   typedef vectorImpl<RenderPassCuller::RenderableNode> VisibleNodeList;
   typedef vectorImpl<ShaderBufferBinding> ShaderBufferList;

   struct RenderPackage {
       vectorImpl<GenericDrawCommand> _drawCommands;
       TextureDataContainer _textureData;
       ShaderBufferList _shaderBuffers;

       bool isCompatible(const RenderPackage& other) const;
   };

   enum class RenderAPI : U32 {
       OpenGL,    ///< 4.x+
       OpenGLES,  ///< 3.x+
       Direct3D,  ///< 11.x+ (not supported yet)
       Vulkan,    ///< not supported yet
       None,      ///< not supported yet
       COUNT
   };

   enum class RenderTarget : U32 {
       SCREEN = 0,
       ANAGLYPH = 1,
       DEPTH = 2,
       COUNT
   };

   struct GPUBlock {
       mat4<F32> _ProjectionMatrix;
       mat4<F32> _ViewMatrix;
       mat4<F32> _ViewProjectionMatrix;
       vec4<F32> _cameraPosition;
       vec4<F32> _ViewPort;
       vec4<F32> _ZPlanesCombined;  // xy - current, zw - main scene
       vec4<F32> _clipPlanes[Config::MAX_CLIP_PLANES];
   };

  public:  // GPU interface
    ErrorCode initRenderingAPI(const vec2<U16>& resolution, I32 argc,
                               char** argv) override;
    void closeRenderingAPI() override;

    inline void setAPI(RenderAPI API) { _API_ID = API; }
    inline RenderAPI getAPI() const { return _API_ID; }

    void idle();
    void beginFrame() override;
    void endFrame() override;

    void enableFog(F32 density, const vec3<F32>& color);
    /// Set all of the needed API specific settings for 2D (Ortho) / 3D
    /// (Perspective) rendering
    void toggle2D(bool state);
    /// Toggle hardware rasterization on or off.
    inline void toggleRasterization(bool state) override;
    /// Query rasterization state
    inline bool rasterizationState();
    /// Change the width of rendered lines to the specified value
    inline void setLineWidth(F32 width) override;
    /// Restore the width of rendered lines to the previously set value
    inline void restoreLineWidth();
    /// Render specified function inside of a viewport of specified dimensions
    /// and position
    inline void renderInViewport(const vec4<I32>& rect,
                                 const DELEGATE_CBK<>& callback);
    void postProcessingEnabled(const bool state);
    /**
     *@brief Returns an immediate mode emulation buffer that can be used to
     *construct geometry in a vertex by vertex manner.
     *@param allowPrimitiveRecycle If false, do not reuse old primitives and do not
     *delete it after x-frames.
     *(Don't use the primitive zombie feature)
     */
    IMPrimitive* getOrCreatePrimitive(bool allowPrimitiveRecycle = true);

    void debugDraw(const SceneRenderState& sceneRenderState);
    IMPrimitive& drawBox3D(const vec3<F32>& min, const vec3<F32>& max,
                           const vec4<U8>& color, F32 lineWidth);
    IMPrimitive& drawLines(const vectorImpl<Line>& lines, F32 lineWidth,
                           const mat4<F32>& globalOffset,
                           const vec4<I32>& viewport,  //<only for ortho mode
                           const bool inViewport = false,
                           const bool disableDepth = false);

    void drawPoints(U32 numPoints, size_t stateHash,
                    ShaderProgram* const shaderProgram);
    void drawGUIElement(GUIElement* guiElement);
    void submitRenderCommand(const GenericDrawCommand& cmd);
    void submitRenderCommands(const vectorImpl<GenericDrawCommand>& cmds);
    void submitIndirectRenderCommand(const GenericDrawCommand& cmd);
    void submitIndirectRenderCommands(const vectorImpl<GenericDrawCommand>& cmds);
    void addToRenderQueue(const RenderPackage& package);
    void flushRenderQueue();
    /// Sets the current render stage.
    ///@param stage Is used to inform the rendering pipeline what we are rendering.
    ///Shadows? reflections? etc
    inline RenderStage setRenderStage(RenderStage stage);
    inline bool isDepthStage() const;

    void setRenderer(RendererType rendererType);
    Renderer& getRenderer() const;

    /// Clipping plane management. All the clipping planes are handled by shader
    /// programs only!
    void updateClipPlanes() override;
    /// disable or enable a clip plane by index
    inline void toggleClipPlane(ClipPlaneIndex index, const bool state);
    /// modify a single clip plane by index
    inline void setClipPlane(ClipPlaneIndex index, const Plane<F32>& p);
    /// set a new list of clipping planes. The old one is discarded
    inline void setClipPlanes(const PlaneList& clipPlanes);
    /// clear all clipping planes
    inline void resetClipPlanes();
    /// Return or create a new state block using the given descriptor.
    /// DO NOT DELETE THE RETURNED STATE BLOCK! GFXDevice handles that!
    size_t getOrCreateStateBlock(const RenderStateBlockDescriptor& descriptor);
    const RenderStateBlockDescriptor& getStateBlockDescriptor(
        size_t renderStateBlockHash) const;

    /// Generate a cubemap from the given position
    /// It renders the entire scene graph (with culling) as default
    /// use the callback param to override the draw function
    void generateCubeMap(
        Framebuffer& cubeMap, const vec3<F32>& pos,
        const DELEGATE_CBK<>& renderFunction, const vec2<F32>& zPlanes,
        RenderStage renderStage = RenderStage::REFLECTION);

    void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat);
    /// Alternative to the normal version of getMatrix
    inline const mat4<F32>& getMatrix(const MATRIX_MODE& mode);

    /// Register a function to be called in the 2D rendering fase of the GFX Flush
    /// routine. Use callOrder for sorting purposes
    inline void add2DRenderFunction(const DELEGATE_CBK<>& callback, U32 callOrder);

    void restoreViewport();
    void setViewport(const vec4<I32>& viewport);
    void changeResolution(U16 w, U16 h) override;
    bool loadInContext(const CurrentContext& context,
                       const DELEGATE_CBK<>& callback);

    void ConstructHIZ();
    /// Save a screenshot in TGA format
    void Screenshot(char* filename);

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
    /// Renders the result of plotting the specified 2D graph
    void plot2DGraph(const Util::GraphPlot2D& plot2D, const vec4<U8>& color);
    /// Renders the result of plotting the specified 3D graph
    void plot3DGraph(const Util::GraphPlot3D& plot3D, const vec4<U8>& color);
    /// Some Scene Node Types are excluded from certain operations (lights
    /// triggers, etc)
    inline bool excludeFromStateChange(const SceneNodeType& currentType) {
        return (_stateExclusionMask & to_int(currentType)) ==
               to_int(currentType);
    }

    inline void setStateChangeExclusionMask(I32 stateMask) {
        _stateExclusionMask = stateMask;
    }

    /// Get the entire list of clipping planes
    inline const PlaneList& getClippingPlanes() const { return _clippingPlanes; }

    /// 2D rendering enabled
    inline bool is2DRendering() const { return _2DRendering; }

    /// Post Processing state
    inline bool postProcessingEnabled() const { return _enablePostProcessing; }

    /// Anaglyph rendering state
    inline bool anaglyphEnabled() const { return _enableAnaglyph; }

    inline void anaglyphEnabled(const bool state) { _enableAnaglyph = state; }

    /// returns the standard state block
    inline size_t getDefaultStateBlock(bool noDepth = false) {
        return noDepth ? _defaultStateNoDepthHash : _defaultStateBlockHash;
    }

    inline Framebuffer* getRenderTarget(RenderTarget target) const {
        return _renderTarget[to_uint(target)];
    }

    RenderDetailLevel shadowDetailLevel() const { return _shadowDetailLevel; }

    void shadowDetailLevel(RenderDetailLevel detailLevel) {
        _shadowDetailLevel = detailLevel;
    }

    inline U32 getFrameCount() const { return FRAME_COUNT; }

    inline I32 getDrawCallCount() const { return FRAME_DRAW_CALLS_PREV; }

    inline void registerDrawCall() { FRAME_DRAW_CALLS++; }

    inline const vec4<I32>& getCurrentViewport() const { return _viewport.top(); }

  public:  // Direct API calls
    inline void setWindowPos(U16 w, U16 h) const { _api->setWindowPos(w, h); }

    /// Hardware specific shader preps (e.g.: OpenGL: init/deinit GLSL-OPT and GLSW)
    inline bool initShaders() override { return _api->initShaders(); }

    inline bool deInitShaders() override { return _api->deInitShaders(); }

    inline IMPrimitive* newIMP() const override { return _api->newIMP(); }

    inline Framebuffer* newFB(bool multisampled = false) const override {
        return _api->newFB(multisampled);
    }

    inline VertexBuffer* newVB() const override { return _api->newVB(); }

    inline PixelBuffer* newPB(
        const PBType& type = PBType::PB_TEXTURE_2D) const override {
        return _api->newPB(type);
    }

    inline GenericVertexData* newGVD(const bool persistentMapped) const override {
        return _api->newGVD(persistentMapped);
    }

    inline Texture* newTextureArray(const bool flipped = false) const override {
        return _api->newTextureArray(flipped);
    }

    inline Texture* newTexture2D(const bool flipped = false) const override {
        return _api->newTexture2D(flipped);
    }

    inline Texture* newTextureCubemap(const bool flipped = false) const override {
        return _api->newTextureCubemap(flipped);
    }

    inline ShaderProgram* newShaderProgram(
        const bool optimise = false) const override {
        return _api->newShaderProgram(optimise);
    }

    inline Shader* newShader(const stringImpl& name, const ShaderType& type,
                             const bool optimise = false) const override {
        return _api->newShader(name, type, optimise);
    }

    inline ShaderBuffer* newSB(
        const stringImpl& bufferName, const bool unbound = false,
        const bool persistentMapped = true) const override {
        return _api->newSB(bufferName, unbound, persistentMapped);
    }

    inline U64 getFrameDurationGPU() override {
        return _api->getFrameDurationGPU();
    }

    inline void uploadDrawCommands(const DrawCommandList& drawCommands,
                                   U32 commandCount) const override {
        _api->uploadDrawCommands(drawCommands, commandCount);
    }

    inline bool makeTexturesResident(const TextureDataContainer& textureData) override {
        return _api->makeTexturesResident(textureData);
    }

    inline bool makeTextureResident(const TextureData& textureData) override {
        return _api->makeTextureResident(textureData);
    }

    inline void setCursorPosition(U16 x, U16 y) const override {
        _api->setCursorPosition(x, y);
    }

  protected:
    inline void changeViewport(const vec4<I32>& newViewport) const override {
        _api->changeViewport(newViewport);
    }

    void threadedLoadCallback() override;

    inline void drawPoints(U32 numPoints) override { 
        uploadGlobalBufferData();
        _api->drawPoints(numPoints); 
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

   protected:
    friend class SceneManager;
    void buildDrawCommands(VisibleNodeList& visibleNodes,
                           SceneRenderState& sceneRenderState,
                           bool refreshNodeData);
    bool batchCommands(GenericDrawCommand& previousIDC,
                       GenericDrawCommand& currentIDC) const;

   private:
    GFXDevice();
    ~GFXDevice();

    void previewDepthBuffer();
    void updateViewportInternal(const vec4<I32>& viewport);
    void forceViewportInternal(const vec4<I32>& viewport);
    void processCommand(const GenericDrawCommand& cmd);
    void processCommands(const vectorImpl<GenericDrawCommand>& cmds);
    /// returns false if there was an invalid state detected that could prevent
    /// rendering
    bool setBufferData(const GenericDrawCommand& cmd);
    /// Upload all relevant buffer data to GPU memory (if needed): node data,
    /// draw commands, gpu matrices, etc
    void uploadGlobalBufferData();
    /// Update the graphics pipeline using the current rendering API with the state
    /// block passed
    inline void activateStateBlock(
        const RenderStateBlock& newBlock,
        RenderStateBlock* const oldBlock) const override {
        _api->activateStateBlock(newBlock, oldBlock);
    }

    inline void drawText(const TextLabel& textLabel,
                         const vec2<I32>& position) override {
        uploadGlobalBufferData();
        _api->drawText(textLabel, position);
    }

    /// Sets the current state block to the one passed as a param
    size_t setStateBlock(size_t stateBlockHash);
    ErrorCode createAPIInstance();

    void processVisibleNode(const RenderPassCuller::RenderableNode& node,
                            NodeData& dataOut);

  private:
    Camera* _cubeCamera;
    Camera* _2DCamera;

    RenderAPIWrapper* _api;
    RenderStage _renderStage;
    I32 _stateExclusionMask;
    bool _drawDebugAxis;
    bool _viewportUpdate;
    vectorImpl<Line> _axisLines;
    vectorImpl<Line> _axisLinesTrasnformed;

  protected:
    RenderAPI _API_ID;
    GPUVendor _GPUVendor;
    GPUState _state;
    std::mutex _loadQueueMutex;
    std::condition_variable _loadQueueCV;
    bool _loadQueueDataReady;
    /* Rendering buffers*/
    std::array<Framebuffer*, to_const_uint(RenderTarget::COUNT)> _renderTarget;
    /*State management */
    RenderStateMap _stateBlockMap;
    bool _stateBlockByDescription;
    size_t _currentStateBlockHash;
    size_t _previousStateBlockHash;
    size_t _defaultStateBlockHash;
    size_t _defaultStateNoDepthHash;  //<The default render state buth with depth
                                      //testing disabled
    size_t _state2DRenderingHash;     //<Special render state for 2D rendering
    size_t _stateDepthOnlyRenderingHash;
    /// The interpolation factor between the current and the last frame
    D32 _interpolationFactor;
    /// Line width management
    F32 _previousLineWidth, _currentLineWidth;
    PlaneList _clippingPlanes;
    bool _enablePostProcessing;
    bool _enableAnaglyph;
    bool _2DRendering;
    bool _rasterizationEnabled;
    bool _useIndirectCommands;
    // number of draw calls (rough estimate)
    I32 FRAME_DRAW_CALLS;
    U32 FRAME_DRAW_CALLS_PREV;
    U32 FRAME_COUNT;
    /// shader used to preview the depth buffer
    ShaderProgram* _previewDepthMapShader;
    ShaderProgram* _HIZConstructProgram;
    /// getMatrix cache
    mat4<F32> _mat4Cache;
    /// Quality settings
    RenderDetailLevel _shadowDetailLevel;

    vectorImpl<std::pair<U32, DELEGATE_CBK<> > > _2dRenderQueue;

    /// Immediate mode emulation
    ShaderProgram* _imShader;  //<The shader used to render VB data
    vectorImpl<IMPrimitive*>
        _imInterfaces;  //<The interface that coverts IM calls to VB data

    /// Current viewport stack
    ViewportStack _viewport;

    GPUBlock _gpuBlock;

    U32 _lastCmdCount;
    vectorAlg::vecSize _lastNodeCount;
    std::array<NodeData, Config::MAX_VISIBLE_NODES + 1> _matricesData;
    std::array<IndirectDrawCommand, Config::MAX_VISIBLE_NODES> _drawCommandsCache;
    typedef vectorImpl<RenderPackage> RenderQueue;
    RenderQueue _renderQueue;
    Time::ProfileTimer* _commandBuildTimer;
    //0 = gfxDataBuffer, 1 = nodeBuffer, 3 = command buffer
    std::array<bool, to_const_uint(GPUBuffer::COUNT)> _buffersDirty;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<ShaderBuffer> _gfxDataBuffer;
    std::unique_ptr<ShaderBuffer> _nodeBuffer;
    GenericDrawCommand _defaultDrawCmd;
END_SINGLETON

};  // namespace Divide

#include "GFXDevice.inl"

#endif
