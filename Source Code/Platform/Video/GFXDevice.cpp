#include "stdafx.h"

#include "config.h"

#include "Headers/GFXDevice.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Headers/PlatformContext.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace TypeUtil {
    const char* renderStageToString(RenderStage stage) {
        switch (stage) {
            case RenderStage::DISPLAY:
                return "DISPLAY";
            case RenderStage::REFLECTION:
                return "REFLECTION";
            case RenderStage::REFRACTION:
                return "REFRACTION";
            case RenderStage::SHADOW:
                return "SHADOW";
        };

        return "error";
    }

    const char* renderPassTypeToString(RenderPassType pass) {
        switch (pass) {
            case RenderPassType::PRE_PASS:
                return "PRE_PASS";
            case RenderPassType::MAIN_PASS:
                return "MAIN_PASS";
            case RenderPassType::OIT_PASS:
                return "OIT_PASS";
        };

        return "error";
    }

    RenderStage stringToRenderStage(const char* stage) {
        if (strcmp(stage, "DISPLAY") == 0) {
            return RenderStage::DISPLAY;
        } else if (strcmp(stage, "REFLECTION") == 0) {
            return RenderStage::REFLECTION;
        } else if (strcmp(stage, "REFRACTION") == 0) {
            return RenderStage::REFRACTION;
        } else if (strcmp(stage, "SHADOW") == 0) {
            return RenderStage::SHADOW;
        }

        return RenderStage::COUNT;
    }

    RenderPassType stringToRenderPassType(const char* pass) {
        if (strcmp(pass, "PRE_PASS") == 0) {
            return RenderPassType::PRE_PASS;
        } else if (strcmp(pass, "MAIN_PASS") == 0) {
            return RenderPassType::MAIN_PASS;
        } else if (strcmp(pass, "OIT_PASS") == 0) {
            return RenderPassType::OIT_PASS;
        }

        return RenderPassType::COUNT;
    }
};

D64 GFXDevice::s_interpolationFactor = 1.0;

GPUVendor GFXDevice::s_GPUVendor = GPUVendor::COUNT;
GPURenderer GFXDevice::s_GPURenderer = GPURenderer::COUNT;

GFXDevice::GFXDevice(Kernel& parent)
   : KernelComponent(parent),
     PlatformContextComponent(parent.platformContext()),
    _api(nullptr),
    _renderer(nullptr),
    _shaderComputeQueue(nullptr),
    _clippingPlanes(Plane<F32>(0, 0, 0, 0))
{
    // Hash values
    _state2DRenderingHash = 0;
    _defaultStateBlockHash = 0;
    _defaultStateNoDepthHash = 0;
    _stateDepthOnlyRenderingHash = 0;
    // Pointers
    _axisGizmo = nullptr;
    _gfxDataBuffer = nullptr;
    _HIZConstructProgram = nullptr;
    _HIZCullProgram = nullptr;
    _renderTargetDraw = nullptr;
    _previewDepthMapShader = nullptr;
    _textRenderShader = nullptr;
    _displayShader = nullptr;
    _debugFrustum = nullptr;
    _debugFrustumPrimitive = nullptr;
    _renderDocManager = nullptr;
    _rtPool = nullptr;
    _textRenderPipeline = nullptr;

    // Integers
    FRAME_COUNT = 0;
    FRAME_DRAW_CALLS = 0;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    LAST_CULL_COUNT = 0;
    // Booleans
    _2DRendering = false;
    // Enumerated Types
    _API_ID = RenderAPI::COUNT;
    
    _viewport.set(-1);

    _lastCommandCount.fill(0);
    _lastNodeCount.fill(0);

    // Red X-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_X_AXIS * 2, UColour(255, 0, 0, 255), 3.0f));
    // Green Y-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Y_AXIS * 2, UColour(0, 255, 0, 255), 3.0f));
    // Blue Z-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Z_AXIS * 2, UColour(0, 0, 255, 255), 3.0f));

    AttribFlags flags;
    flags.fill(true);
    VertexBuffer::setAttribMasks(static_cast<size_t>(RenderStagePass::count()), flags);

    // Don't (currently) need these for shadow passes
    flags[to_base(AttribLocation::COLOR)] = false;
    flags[to_base(AttribLocation::TANGENT)] = false;
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        VertexBuffer::setAttribMask(RenderStagePass(static_cast<RenderStage>(stage), RenderPassType::PRE_PASS).index(), flags);
    }
    flags[to_base(AttribLocation::NORMAL)] = false;
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        VertexBuffer::setAttribMask(RenderStagePass(RenderStage::SHADOW, static_cast<RenderPassType>(pass)).index(), flags);
    }
    for (U8 i = 0; i < to_base(Frustum::FrustPlane::COUNT); ++i) {
        _frustumPlanes[i].set(0.0f);
    }
}

GFXDevice::~GFXDevice()
{
}

/// Generate a cube texture and store it in the provided RenderTarget
void GFXDevice::generateCubeMap(RenderTargetID cubeMap,
                                const U16 arrayOffset,
                                const vec3<F32>& pos,
                                const vec2<F32>& zPlanes,
                                RenderStagePass stagePass,
                                U32 passIndex,
                                GFX::CommandBuffer& bufferInOut,
                                SceneGraphNode* sourceNode,
                                Camera* camera) {

    if (!camera) {
        camera = Camera::utilityCamera(Camera::UtilityCamera::CUBE);
    }

    // Only the first colour attachment or the depth attachment is used for now
    // and it must be a cube map texture
    RenderTarget& cubeMapTarget = _rtPool->renderTarget(cubeMap);
    const RTAttachment& colourAttachment = cubeMapTarget.getAttachment(RTAttachmentType::Colour, 0);
    const RTAttachment& depthAttachment = cubeMapTarget.getAttachment(RTAttachmentType::Depth, 0);
    // Colour attachment takes precedent over depth attachment
    bool hasColour = colourAttachment.used();
    bool hasDepth = depthAttachment.used();
    // Everyone's innocent until proven guilty
    bool isValidFB = true;
    if (hasColour) {
        // We only need the colour attachment
        isValidFB = (colourAttachment.texture()->getDescriptor().isCubeTexture());
    } else {
        // We don't have a colour attachment, so we require a cube map depth
        // attachment
        isValidFB = hasDepth && depthAttachment.texture()->getDescriptor().isCubeTexture();
    }
    // Make sure we have a proper render target to draw to
    if (!isValidFB) {
        // Future formats must be added later (e.g. cube map arrays)
        Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE_INVALID_FB_CUBEMAP")));
        return;
    }
    // No dual-paraboloid rendering here. Just draw once for each face.
    static vec3<F32> TabUp[6] = {WORLD_Y_NEG_AXIS, WORLD_Y_NEG_AXIS,
                                 WORLD_Z_AXIS,     WORLD_Z_NEG_AXIS,
                                 WORLD_Y_NEG_AXIS, WORLD_Y_NEG_AXIS};
    // Get the center and up vectors for each cube face
    vec3<F32> TabCenter[6] = {vec3<F32>( 1.0f,  0.0f,  0.0f),
                              vec3<F32>(-1.0f,  0.0f,  0.0f),
                              vec3<F32>( 0.0f,  1.0f,  0.0f),
                              vec3<F32>( 0.0f, -1.0f,  0.0f),
                              vec3<F32>( 0.0f,  0.0f,  1.0f),
                              vec3<F32>( 0.0f,  0.0f, -1.0f)};

    // Set a 90 degree vertical FoV perspective projection
    camera->setProjection(1.0f, 90.0f, zPlanes);

    // Enable our render target
    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = cubeMap;
    beginRenderPassCmd._name = "GENERATE_CUBE_MAP";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    // For each of the environment's faces (TOP, DOWN, NORTH, SOUTH, EAST, WEST)

    RenderPassManager& passMgr = parent().renderPassManager();
    RenderPassManager::PassParams params;
    params._sourceNode = sourceNode;
    params._camera = camera;
    params._stage = stagePass._stage;
    params._pass = stagePass._passType;
    params._target = cubeMap;
    params._passIndex = passIndex;
    // We do our own binding
    params._bindTargets = false;
    params._passVariant = stagePass._variant;

    GFX::BeginRenderSubPassCommand beginRenderSubPassCmd;
    GFX::EndRenderSubPassCommand endRenderSubPassCommand;
    RenderTarget::DrawLayerParams drawParams;
    drawParams._type = hasColour ? RTAttachmentType::Colour : RTAttachmentType::Depth;
    drawParams._index = 0;

    for (U8 i = 0; i < 6; ++i) {
        // Draw to the current cubemap face
        drawParams._layer = i + arrayOffset;
        beginRenderSubPassCmd._writeLayers.resize(1, drawParams);
        GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);

        // Point our camera to the correct face
        camera->lookAt(pos, TabCenter[i], TabUp[i]);
        // Pass our render function to the renderer
        params._passIndex = i;
        passMgr.doCustomPass(params, bufferInOut);
        GFX::EnqueueCommand(bufferInOut, endRenderSubPassCommand);
    }

    // Resolve our render target
    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

void GFXDevice::generateDualParaboloidMap(RenderTargetID targetBuffer,
                                          const U16 arrayOffset,
                                          const vec3<F32>& pos,
                                          const vec2<F32>& zPlanes,
                                          RenderStagePass stagePass,
                                          U32 passIndex,
                                          GFX::CommandBuffer& bufferInOut,
                                          SceneGraphNode* sourceNode,
                                          Camera* camera)
{
    if (!camera) {
        camera = Camera::utilityCamera(Camera::UtilityCamera::DUAL_PARABOLOID);
    }

    RenderTarget& paraboloidTarget = _rtPool->renderTarget(targetBuffer);
    const RTAttachment& colourAttachment = paraboloidTarget.getAttachment(RTAttachmentType::Colour, 0);
    const RTAttachment& depthAttachment = paraboloidTarget.getAttachment(RTAttachmentType::Depth, 0);
    // Colour attachment takes precedent over depth attachment
    bool hasColour = colourAttachment.used();
    bool hasDepth = depthAttachment.used();
    bool isValidFB = true;
    if (hasColour) {
        // We only need the colour attachment
        isValidFB = colourAttachment.texture()->getDescriptor().isArrayTexture();
    } else {
        // We don't have a colour attachment, so we require a cube map depth attachment
        isValidFB = hasDepth && depthAttachment.texture()->getDescriptor().isArrayTexture();
    }
    // Make sure we have a proper render target to draw to
    if (!isValidFB) {
        // Future formats must be added later (e.g. cube map arrays)
        Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE_INVALID_FB_DP")));
        return;
    }

    // Set a 90 degree vertical FoV perspective projection
    camera->setProjection(1.0f, 180.0f, zPlanes);

    RenderPassManager& passMgr = parent().renderPassManager();
    RenderPassManager::PassParams params;
    params._sourceNode = sourceNode;
    params._camera = camera;
    params._stage = stagePass._stage;
    params._target = targetBuffer;
    params._bindTargets = false;
    params._passVariant = stagePass._variant;
    params._pass = stagePass._passType;
    params._passIndex = passIndex;
    // Enable our render target

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = targetBuffer;
    beginRenderPassCmd._name = "GENERATE_DUAL_PARABOLOID_MAP";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::BeginRenderSubPassCommand beginRenderSubPassCmd;
    GFX::EndRenderSubPassCommand endRenderSubPassCommand;

    RenderTarget::DrawLayerParams drawParams;
    drawParams._type = hasColour ? RTAttachmentType::Colour : RTAttachmentType::Depth;
    drawParams._index = 0;

    for (U8 i = 0; i < 2; ++i) {
        drawParams._layer = i + arrayOffset;
        beginRenderSubPassCmd._writeLayers.resize(1, drawParams);
        GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);

        // Point our camera to the correct face
        camera->lookAt(pos, (i == 0 ? WORLD_Z_NEG_AXIS : WORLD_Z_AXIS));
        // And generated required matrices
        // Pass our render function to the renderer
        params._passIndex = i;
        passMgr.doCustomPass(params, bufferInOut);
        GFX::EnqueueCommand(bufferInOut, endRenderSubPassCommand);
    }
    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

void GFXDevice::increaseResolution() {
    stepResolution(true);
}

void GFXDevice::decreaseResolution() {
    stepResolution(false);
}

void GFXDevice::stepResolution(bool increment) {
    auto compare = [](const vec2<U16>& a, const vec2<U16>& b) -> bool {
        return a.x > b.x || a.y > b.y;
    };

    WindowManager& winManager = _parent.platformContext().app().windowManager();

    vector<GPUState::GPUVideoMode>::const_iterator it;
    const vector<GPUState::GPUVideoMode>& displayModes = _state.getDisplayModes(winManager.getMainWindow().currentDisplayIndex());

    bool found = false;
    vec2<U16> foundRes;
    if (increment) {
        for (const GPUState::GPUVideoMode& mode : reverse(displayModes)) {
            const vec2<U16>& res = mode._resolution;
            if (compare(res, _renderingResolution)) {
                found = true;
                foundRes.set(res);
                break;
            }
        }
    } else {
        for (const GPUState::GPUVideoMode& mode : displayModes) {
            const vec2<U16>& res = mode._resolution;
            if (compare(_renderingResolution, res)) {
                found = true;
                foundRes.set(res);
                break;
            }
        }
    }
    
    if (found) {
        _resolutionChangeQueued.first.set(foundRes);
        _resolutionChangeQueued.second = true;
    }
}

void GFXDevice::toggleFullScreen() {
    WindowManager& winManager = _parent.platformContext().app().windowManager();

    switch (winManager.getMainWindow().type()) {
        case WindowType::WINDOW:
            winManager.getMainWindow().changeType(WindowType::FULLSCREEN_WINDOWED);
            break;
        case WindowType::FULLSCREEN_WINDOWED:
            winManager.getMainWindow().changeType(WindowType::FULLSCREEN);
            break;
        case WindowType::FULLSCREEN:
            winManager.getMainWindow().changeType(WindowType::WINDOW);
            break;
    };
}

/// The main entry point for any resolution change request
void GFXDevice::onSizeChange(const SizeChangeParams& params) {
    if (params.winGUID != context().app().windowManager().getMainWindow().getGUID()) {
        return;
    }

    U16 w = params.width;
    U16 h = params.height;

    if (!params.isWindowResize) {
        // Update resolution only if it's different from the current one.
        // Avoid resolution change on minimize so we don't thrash render targets
        if (w < 1 || h < 1 || _renderingResolution == vec2<U16>(w, h)) {
            return;
        }

        _renderingResolution.set(w, h);

        // Update render targets with the new resolution
        _rtPool->resizeTargets(RenderTargetUsage::SCREEN, w, h);
        _rtPool->resizeTargets(RenderTargetUsage::HI_Z, w, h);
        _rtPool->resizeTargets(RenderTargetUsage::OIT, w, h);
        if (Config::Build::ENABLE_EDITOR) {
            _rtPool->resizeTargets(RenderTargetUsage::EDITOR, w, h);
        }

        U16 reflectRes = std::max(w, h) / Config::REFLECTION_TARGET_RESOLUTION_DOWNSCALE_FACTOR;
        _rtPool->resizeTargets(RenderTargetUsage::HI_Z_REFLECT, reflectRes, reflectRes);
        _rtPool->resizeTargets(RenderTargetUsage::REFLECTION_PLANAR, reflectRes, reflectRes);
        _rtPool->resizeTargets(RenderTargetUsage::REFRACTION_PLANAR, reflectRes, reflectRes);
        _rtPool->resizeTargets(RenderTargetUsage::REFLECTION_CUBE, reflectRes, reflectRes);
        _rtPool->resizeTargets(RenderTargetUsage::REFRACTION_CUBE, reflectRes, reflectRes);

        _prevDepthBuffer->resize(NULL, vec2<U16>(w, h));

        // Update post-processing render targets and buffers
        postFX().updateResolution(w, h);

        // Update the 2D camera so it matches our new rendering viewport
        Camera::utilityCamera(Camera::UtilityCamera::_2D)->setProjection(vec4<F32>(0, to_F32(w), 0, to_F32(h)), vec2<F32>(-1, 1));
        Camera::utilityCamera(Camera::UtilityCamera::_2D_FLIP_Y)->setProjection(vec4<F32>(0, to_F32(w), to_F32(h), 0), vec2<F32>(-1, 1));
    }

    fitViewportInWindow(w, h);
}

void GFXDevice::fitViewportInWindow(U16 w, U16 h) {
    F32 currentAspectRatio = renderingAspectRatio();

    I32 left = 0, bottom = 0;
    I32 newWidth = w;
    I32 newHeight = h;

    I32 tempWidth = to_I32(h * currentAspectRatio);
    I32 tempHeight = to_I32(w / currentAspectRatio);

    F32 newAspectRatio = to_F32(tempWidth) / tempHeight;

    if (newAspectRatio <= currentAspectRatio) {
        newWidth = tempWidth;
        left = to_I32((w - newWidth) * 0.5f);
    } else {
        newHeight = tempHeight;
        bottom = to_I32((h - newHeight) * 0.5f);
    }
    
    context().activeWindow().renderingViewport(Rect<I32>(left, bottom, newWidth, newHeight));
}

/// set a new list of clipping planes. The old one is discarded
void GFXDevice::setClipPlanes(const FrustumClipPlanes& clipPlanes) {
    static_assert(std::is_same<std::remove_reference<decltype(*(_gpuBlock._data._clipPlanes))>::type, vec4<F32>>::value, "GFXDevice error: invalid clip plane type!");
    static_assert(sizeof(vec4<F32>) == sizeof(Plane<F32>), "GFXDevice error: clip plane size mismatch!");

    if (clipPlanes._planes != _clippingPlanes._planes)
    {
        _clippingPlanes = clipPlanes;

        memcpy(&_gpuBlock._data._clipPlanes[0],
               _clippingPlanes._planes.data(),
               sizeof(vec4<F32>) * to_base(ClipPlaneIndex::COUNT));

        _gpuBlock._needsUpload = true;
    }
}

void GFXDevice::renderFromCamera(const CameraSnapshot& cameraSnapshot, RenderStage stage) {
    // Tell the Rendering API to draw from our desired PoV
    if (_activeCameraSnapshot != cameraSnapshot) {
        _activeCameraSnapshot = cameraSnapshot;

        GFXShaderData::GPUData& data = _gpuBlock._data;
        data._ViewMatrix.set(cameraSnapshot._viewMatrix);

        if (cameraSnapshot._projectionMatrix != data._ProjectionMatrix) {
            data._ProjectionMatrix.set(cameraSnapshot._projectionMatrix);
            data._ProjectionMatrix.getInverse(data._InvProjectionMatrix);
        }

        F32 FoV = cameraSnapshot._FoV;
        data._cameraPosition.set(cameraSnapshot._eye, cameraSnapshot._aspectRatio);
        data._renderProperties.xy(cameraSnapshot._zPlanes);
        data._renderProperties.z = FoV;
        data._renderProperties.w = to_F32(to_base(stage));

        mat4<F32>::Multiply(data._ViewMatrix, data._ProjectionMatrix, data._ViewProjectionMatrix);
        Frustum::computePlanes(GetInverse(data._ViewProjectionMatrix), _frustumPlanes);
        _gpuBlock._needsUpload = true;
    }
}

/// Update the rendering viewport
bool GFXDevice::setViewport(const Rect<I32>& viewport) {
    // Change the viewport on the Rendering API level
    if (_api->setViewport(viewport)) {
    // Update the buffer with the new value
        _gpuBlock._data._ViewPort.set(viewport.x, viewport.y, viewport.z, viewport.w);
        _gpuBlock._needsUpload = true;
        _viewport.set(viewport);

        return true;
    }

    return false;
}

/// Transform our depth buffer to a HierarchicalZ buffer (for occlusion queries and screen space reflections)
/// Based on RasterGrid implementation: http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/
/// Modified with nVidia sample code: https://github.com/nvpro-samples/gl_occlusion_culling
const Texture_ptr& GFXDevice::constructHIZ(RenderTargetID depthBuffer, RenderTargetID HiZTarget, GFX::CommandBuffer& cmdBufferInOut) const {
    static bool firstRun = true;

    // We use a special shader that downsamples the buffer
    // We will use a state block that disables colour writes as we will render only a depth image,
    // disables depth testing but allows depth writes
    // Set the depth buffer as the currently active render target
    RTDrawDescriptor depthOnlyTarget;
    depthOnlyTarget.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
    depthOnlyTarget.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
    depthOnlyTarget.disableState(RTDrawDescriptor::State::CHANGE_VIEWPORT);
    depthOnlyTarget.drawMask().disableAll();
    depthOnlyTarget.drawMask().setEnabled(RTAttachmentType::Depth, 0, true);

    RenderStateBlock HiZState;
    HiZState.setZFunc(ComparisonFunction::ALWAYS);

    PipelineDescriptor pipelineDesc;
    pipelineDesc._stateHash = HiZState.getHash();
    pipelineDesc._shaderProgramHandle = _HIZConstructProgram->getID();

    // The depth buffer's resolution should be equal to the screen's resolution
    RenderTarget& renderTarget = _rtPool->renderTarget(HiZTarget);
    U16 width = renderTarget.getWidth();
    U16 height = renderTarget.getHeight();
    U16 level = 0;
    U16 dim = width > height ? width : height;

    // Store the current width and height of each mip
    Rect<I32> previousViewport(_viewport);

    // Bind the depth texture to the first texture unit
    const Texture_ptr& depth = renderTarget.getAttachment(RTAttachmentType::Depth, 0).texture();
    if (depth->getDescriptor().automaticMipMapGeneration()) {
        return depth;
    }

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = to_I32(depthBuffer._index);
    beginDebugScopeCmd._scopeName = "Construct Hi-Z";
    GFX::EnqueueCommand(cmdBufferInOut, beginDebugScopeCmd);

    // Blit the depth buffer to the HiZ target
    GFX::BlitRenderTargetCommand blitDepthBufferCmd;
    blitDepthBufferCmd._source = depthBuffer;
    blitDepthBufferCmd._destination = HiZTarget;
    blitDepthBufferCmd._blitDepth.emplace_back();
    GFX::EnqueueCommand(cmdBufferInOut, blitDepthBufferCmd);

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = HiZTarget;
    beginRenderPassCmd._descriptor = depthOnlyTarget;
    beginRenderPassCmd._name = "CONSTRUCT_HI_Z";
    GFX::EnqueueCommand(cmdBufferInOut, beginRenderPassCmd);

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = newPipeline(pipelineDesc);
    GFX::EnqueueCommand(cmdBufferInOut, pipelineCmd);

    GFX::SetViewportCommand viewportCommand;
    GFX::SendPushConstantsCommand pushConstantsCommand;
    GFX::EndRenderSubPassCommand endRenderSubPassCmd;

    GFX::BeginRenderSubPassCommand beginRenderSubPassCmd;
    beginRenderSubPassCmd._validateWriteLevel = firstRun;

    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;

    // for i > 0, use texture views?
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.setTexture(depth->getData(), to_U8(ShaderProgram::TextureUsage::DEPTH));
    GFX::EnqueueCommand(cmdBufferInOut, descriptorSetCmd);

    // We skip the first level as that's our full resolution image
    U16 twidth = width;
    U16 theight = height;
    bool wasEven = false;

    while (dim) {
        if (level) {
            twidth = twidth < 1 ? 1 : twidth;
            theight = theight < 1 ? 1 : theight;

            // Bind next mip level for rendering but first restrict fetches only to previous level
            beginRenderSubPassCmd._mipWriteLevel = level;
            GFX::EnqueueCommand(cmdBufferInOut, beginRenderSubPassCmd);

            // Update the viewport with the new resolution
            viewportCommand._viewport.set(0, 0, twidth, theight);
            GFX::EnqueueCommand(cmdBufferInOut, viewportCommand);

            pushConstantsCommand._constants.set("depthInfo", GFX::PushConstantType::IVEC2, vec2<I32>(level - 1, wasEven ? 1 : 0));
            GFX::EnqueueCommand(cmdBufferInOut, pushConstantsCommand);

            // Dummy draw command as the full screen quad is generated completely in the vertex shader
            GFX::DrawCommand drawCmd;
            drawCmd._drawCommands.push_back(triangleCmd);
            GFX::EnqueueCommand(cmdBufferInOut, drawCmd);

            GFX::EnqueueCommand(cmdBufferInOut, endRenderSubPassCmd);
        }

        // Calculate next viewport size
        wasEven = (twidth % 2 == 0) && (theight % 2 == 0);
        dim /= 2;
        twidth /= 2;
        theight /= 2;
        level++;
    }

    // Restore mip level
    beginRenderSubPassCmd._mipWriteLevel = 0;
    GFX::EnqueueCommand(cmdBufferInOut, beginRenderSubPassCmd);
    GFX::EnqueueCommand(cmdBufferInOut, endRenderSubPassCmd);

    viewportCommand._viewport.set(previousViewport);
    GFX::EnqueueCommand(cmdBufferInOut, viewportCommand);
 
    // Unbind the render target
    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(cmdBufferInOut, endRenderPassCmd);
    
    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(cmdBufferInOut, endDebugScopeCmd);

    firstRun = false;

    return depth;
}

void GFXDevice::updateCullCount(const RenderPass::BufferData& bufferData, GFX::CommandBuffer& cmdBufferInOut) {
    if (_queueCullRead) {
        if (bufferData._cullCounter != nullptr) {
            GFX::ReadBufferDataCommand readAtomicCounter;
            readAtomicCounter._buffer = bufferData._cullCounter;
            readAtomicCounter._target = &LAST_CULL_COUNT;
            readAtomicCounter._offsetElementCount = 0;
            readAtomicCounter._elementCount = 1;
            GFX::EnqueueCommand(cmdBufferInOut, readAtomicCounter);
        }

        _queueCullRead = false;
    }
}

Renderer& GFXDevice::getRenderer() const {
    DIVIDE_ASSERT(_renderer != nullptr, "GFXDevice error: Renderer requested but not created!");
    return *_renderer;
}

ShaderComputeQueue& GFXDevice::shaderComputeQueue() {
    assert(_shaderComputeQueue != nullptr);
    return *_shaderComputeQueue;
}

const ShaderComputeQueue& GFXDevice::shaderComputeQueue() const {
    assert(_shaderComputeQueue != nullptr);
    return *_shaderComputeQueue;
}

/// Extract the pixel data from the main render target's first colour attachment and save it as a TGA image
void GFXDevice::Screenshot(const stringImpl& filename) {
    // Get the screen's resolution
    RenderTarget& screenRT = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    U16 width = screenRT.getWidth();
    U16 height = screenRT.getHeight();
    // Allocate sufficiently large buffers to hold the pixel data
    U32 bufferSize = width * height * 4;
    U8* imageData = MemoryManager_NEW U8[bufferSize];
    // Read the pixels from the main render target (RGBA16F)
    screenRT.readData(GFXImageFormat::RGBA, GFXDataFormat::UNSIGNED_BYTE, imageData);
    // Save to file
    ImageTools::SaveSeries(filename,
                           vec2<U16>(width, height),
                           32,
                           imageData);
    // Delete local buffers
    MemoryManager::DELETE_ARRAY(imageData);
}

};
