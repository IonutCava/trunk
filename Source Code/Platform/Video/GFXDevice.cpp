#include "config.h"

#include "Headers/GFXDevice.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "Rendering/Headers/TiledForwardShadingRenderer.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"

#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

D64 GFXDevice::s_interpolationFactor = 1.0;


GFXDevice::GFXDevice(Kernel& parent)
   : KernelComponent(parent), 
    _api(nullptr),
    _renderer(nullptr),
    _shaderComputeQueue(nullptr),
    _commandPool(Config::MAX_DRAW_COMMANDS_IN_FLIGHT),
    _renderStage(RenderStage::DISPLAY),
    _prevRenderStage(RenderStage::COUNT),
    _commandBuildTimer(Time::ADD_TIMER("Command Generation Timer"))
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
    _displayShader = nullptr;
    _debugFrustum = nullptr;
    _debugFrustumPrimitive = nullptr;
    _renderDocManager = nullptr;
    _rtPool = nullptr;

    // Integers
    _historyIndex = 0;
    FRAME_COUNT = 0;
    FRAME_DRAW_CALLS = 0;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    // Cameras
    _2DCamera = nullptr;
    _cubeCamera = nullptr;
    _dualParaboloidCamera = nullptr;
    // Booleans
    _2DRendering = false;
    _isPrePassStage = false;
    // Enumerated Types
    _shadowDetailLevel = RenderDetailLevel::HIGH;
    _GPUVendor = GPUVendor::COUNT;
    _GPURenderer = GPURenderer::COUNT;
    _API_ID = RenderAPI::COUNT;
    // Clipping planes
    _clippingPlanes.resize(to_const_uint(Frustum::FrustPlane::COUNT), Plane<F32>(0, 0, 0, 0));
    // To allow calls to "setBaseViewport"
    _viewport.push(vec4<I32>(-1));

    _lastCommandCount.fill(0);
    _lastNodeCount.fill(0);
    // Red X-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_X_AXIS * 2, vec4<U8>(255, 0, 0, 255), 3.0f));
    // Green Y-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Y_AXIS * 2, vec4<U8>(0, 255, 0, 255), 3.0f));
    // Blue Z-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Z_AXIS * 2, vec4<U8>(0, 0, 255, 255), 3.0f));

    VertexBuffer::AttribFlags flags;
    flags.fill(true);
    VertexBuffer::setAttribMasks(flags);

    // Don't (currently) need these for shadow passes
    flags[to_const_uint(VertexBuffer::VertexAttribute::ATTRIB_COLOR)] = false;
    flags[to_const_uint(VertexBuffer::VertexAttribute::ATTRIB_TANGENT)] = false;
    VertexBuffer::setAttribMask(RenderStage::Z_PRE_PASS, flags);
    flags[to_const_uint(VertexBuffer::VertexAttribute::ATTRIB_NORMAL)] = false;
    VertexBuffer::setAttribMask(RenderStage::SHADOW, flags);
}

GFXDevice::~GFXDevice()
{
}

/// Generate a cube texture and store it in the provided RenderTarget
void GFXDevice::generateCubeMap(RenderTarget& cubeMap,
                                const U16 arrayOffset,
                                const vec3<F32>& pos,
                                const vec2<F32>& zPlanes,
                                RenderStage renderStage,
                                U32 passIndex) {
    // Only the first colour attachment or the depth attachment is used for now
    // and it must be a cube map texture
    const RTAttachment& colourAttachment = cubeMap.getAttachment(RTAttachment::Type::Colour, 0, false);
    const RTAttachment& depthAttachment = cubeMap.getAttachment(RTAttachment::Type::Depth, 0, false);
    // Colour attachment takes precedent over depth attachment
    bool hasColour = colourAttachment.used();
    bool hasDepth = depthAttachment.used();
    // Everyone's innocent until proven guilty
    bool isValidFB = true;
    if (hasColour) {
        // We only need the colour attachment
        isValidFB = (colourAttachment.descriptor().isCubeTexture());
    } else {
        // We don't have a colour attachment, so we require a cube map depth
        // attachment
        isValidFB = hasDepth && depthAttachment.descriptor().isCubeTexture();
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
    vec3<F32> TabCenter[6] = {vec3<F32>(pos.x + 1.0f, pos.y, pos.z),
                              vec3<F32>(pos.x - 1.0f, pos.y, pos.z),
                              vec3<F32>(pos.x, pos.y + 1.0f, pos.z),
                              vec3<F32>(pos.x, pos.y - 1.0f, pos.z),
                              vec3<F32>(pos.x, pos.y, pos.z + 1.0f),
                              vec3<F32>(pos.x, pos.y, pos.z - 1.0f)};

    // Set a 90 degree vertical FoV perspective projection
    _cubeCamera->setProjection(1.0f, 90.0f, zPlanes);
    // Set the desired render stage, remembering the previous one
    RenderStage prevRenderStage = setRenderStage(renderStage);
    // Enable our render target
    cubeMap.begin(RenderTarget::defaultPolicy());
    // For each of the environment's faces (TOP, DOWN, NORTH, SOUTH, EAST, WEST)

    RenderPassManager& passMgr = parent().renderPassManager();
    RenderPassManager::PassParams params;
    params.doPrePass = renderStage != RenderStage::SHADOW;
    params.occlusionCull = params.doPrePass;
    params.camera = _cubeCamera;
    params.stage = renderStage;

    for (U8 i = 0; i < 6; ++i) {
        // Draw to the current cubemap face
        cubeMap.drawToFace(hasColour ? RTAttachment::Type::Colour
                                     : RTAttachment::Type::Depth,
                           0,
                           i + arrayOffset);
        // Point our camera to the correct face
        _cubeCamera->lookAt(pos, TabCenter[i], TabUp[i]);
        // Pass our render function to the renderer
        params.pass = passIndex + i;
        passMgr.doCustomPass(params);
    }

    // Resolve our render target
    cubeMap.end();
    // Return to our previous rendering stage
    setRenderStage(prevRenderStage);
}

void GFXDevice::generateDualParaboloidMap(RenderTarget& targetBuffer,
                                          const U16 arrayOffset,
                                          const vec3<F32>& pos,
                                          const vec2<F32>& zPlanes,
                                          RenderStage renderStage,
                                          U32 passIndex)
{
    const RTAttachment& colourAttachment = targetBuffer.getAttachment(RTAttachment::Type::Colour, 0, false);
    const RTAttachment& depthAttachment = targetBuffer.getAttachment(RTAttachment::Type::Depth, 0, false);
    // Colour attachment takes precedent over depth attachment
    bool hasColour = colourAttachment.used();
    bool hasDepth = depthAttachment.used();
    bool isValidFB = true;
    if (hasColour) {
        // We only need the colour attachment
        isValidFB = colourAttachment.descriptor().isArrayTexture();
    } else {
        // We don't have a colour attachment, so we require a cube map depth attachment
        isValidFB = hasDepth && depthAttachment.descriptor().isArrayTexture();
    }
    // Make sure we have a proper render target to draw to
    if (!isValidFB) {
        // Future formats must be added later (e.g. cube map arrays)
        Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE_INVALID_FB_DP")));
        return;
    }

    // Set a 90 degree vertical FoV perspective projection
    _dualParaboloidCamera->setProjection(1.0f, 180.0f, zPlanes);
    // Set the desired render stage, remembering the previous one
    RenderStage prevRenderStage = setRenderStage(renderStage);

    RenderPassManager& passMgr = parent().renderPassManager();
    RenderPassManager::PassParams params;
    params.doPrePass = renderStage != RenderStage::SHADOW;
    params.occlusionCull = params.doPrePass;
    params.camera = _dualParaboloidCamera;
    params.stage = renderStage;

    // Enable our render target
    targetBuffer.begin(RenderTarget::defaultPolicy());
        for (U8 i = 0; i < 2; ++i) {
            targetBuffer.drawToLayer(hasColour ? RTAttachment::Type::Colour
                                               : RTAttachment::Type::Depth,
                                     0,
                                     i + arrayOffset);
            // Point our camera to the correct face
            _dualParaboloidCamera->lookAt(pos, (i == 0 ? WORLD_Z_NEG_AXIS : WORLD_Z_AXIS) + pos, WORLD_Y_AXIS);
            // And generated required matrices
            // Pass our render function to the renderer
            params.pass = passIndex + i;
            passMgr.doCustomPass(params);
        }
    targetBuffer.end();
    // Return to our previous rendering stage
    setRenderStage(prevRenderStage);
}

void GFXDevice::increaseResolution() {
    const WindowManager& winManager = Application::instance().windowManager();
    const vec2<U16>& resolution = winManager.getActiveWindow().getDimensions();
    const vectorImpl<GPUState::GPUVideoMode>& displayModes = _state.getDisplayModes(winManager.targetDisplay());

    vectorImpl<GPUState::GPUVideoMode>::const_reverse_iterator it;
    for (it = std::rbegin(displayModes); it != std::rend(displayModes); ++it) {
        const vec2<U16>& tempResolution = it->_resolution;

        if (resolution.width < tempResolution.width &&
            resolution.height < tempResolution.height) {
            WindowManager& winMgr = Application::instance().windowManager();
            winMgr.handleWindowEvent(WindowEvent::RESOLUTION_CHANGED,
                                     winMgr.getActiveWindow().getGUID(),
                                     to_int(tempResolution.width),
                                     to_int(tempResolution.height));
            return;
        }
    }
}

void GFXDevice::decreaseResolution() {
    const WindowManager& winManager = Application::instance().windowManager();
    const vec2<U16>& resolution = winManager.getActiveWindow().getDimensions();
    const vectorImpl<GPUState::GPUVideoMode>& displayModes = _state.getDisplayModes(winManager.targetDisplay());
    
    vectorImpl<GPUState::GPUVideoMode>::const_iterator it;
    for (it = std::begin(displayModes); it != std::end(displayModes); ++it) {
        const vec2<U16>& tempResolution = it->_resolution;
        if (resolution.width > tempResolution.width &&
            resolution.height > tempResolution.height) {
            WindowManager& winMgr = Application::instance().windowManager();
            winMgr.handleWindowEvent(WindowEvent::RESOLUTION_CHANGED,
                                     winMgr.getActiveWindow().getGUID(),
                                     to_int(tempResolution.width),
                                     to_int(tempResolution.height));
            return;
        }
    }
}

void GFXDevice::toggleFullScreen() {
    WindowManager& winManager = Application::instance().windowManager();
    switch (winManager.getActiveWindow().type()) {
        case WindowType::WINDOW:
        case WindowType::SPLASH:
            winManager.getActiveWindow().type(WindowType::FULLSCREEN_WINDOWED);
            break;
        case WindowType::FULLSCREEN_WINDOWED:
            winManager.getActiveWindow().type(WindowType::FULLSCREEN);
            break;
        case WindowType::FULLSCREEN:
            winManager.getActiveWindow().type(WindowType::WINDOW);
            break;
    };
}

/// The main entry point for any resolution change request
void GFXDevice::onChangeResolution(U16 w, U16 h) {
    RenderTarget& screenRT = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    // Update resolution only if it's different from the current one.
    // Avoid resolution change on minimize so we don't thrash render targets
    if ((w == screenRT.getWidth() &&
         h == screenRT.getHeight()) ||
        !(w > 1 && h > 1)) {
        return;
    }

    // Update render targets with the new resolution
    _rtPool->resizeTargets(RenderTargetUsage::SCREEN, w, h);

    for (Texture_ptr& tex : _prevDepthBuffers) {
        vec2<U16> mipLevel(0,
            tex->getDescriptor().getSampler().generateMipMaps()
            ? 1 + Texture::computeMipCount(w, h)
            : 1);

        tex->resize(NULL, vec2<U16>(w, h), mipLevel);
    }

    // Update post-processing render targets and buffers
    PostFX::instance().updateResolution(w, h);

    // Update the 2D camera so it matches our new rendering viewport
    _2DCamera->setProjection(vec4<F32>(0, to_float(w), 0, to_float(h)), vec2<F32>(-1, 1));
}

/// Return a GFXDevice specific matrix or a derivative of it
void GFXDevice::getMatrix(const MATRIX& mode, mat4<F32>& mat) const {
    mat.set(getMatrixInternal(mode));
}

mat4<F32>& GFXDevice::getMatrixInternal(const MATRIX& mode) {
    // The matrix names are self-explanatory
    switch (mode) {
        case  MATRIX::VIEW_PROJECTION:
            return _gpuBlock._data._ViewProjectionMatrix;
        case MATRIX::VIEW:
            return _gpuBlock._data._ViewMatrix;
        case MATRIX::PROJECTION:
            return _gpuBlock._data._ProjectionMatrix;
        case MATRIX::VIEW_INV: 
            return _gpuBlock._viewMatrixInv;
        case MATRIX::PROJECTION_INV:
            return _gpuBlock._data._InvProjectionMatrix;
        case MATRIX::VIEW_PROJECTION_INV:
            return _gpuBlock._viewProjMatrixInv;
        case MATRIX::TEXTURE:
            Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_MATRIX_ACCESS")));
            break;
        default:
            DIVIDE_ASSERT(false, "GFXDevice error: attempted to query an invalid matrix target!");
            break;
    };
    
    return MAT4_IDENTITY;
}

const mat4<F32>& GFXDevice::getMatrixInternal(const MATRIX& mode) const {
    // The matrix names are self-explanatory
    switch (mode) {
        case  MATRIX::VIEW_PROJECTION:
            return _gpuBlock._data._ViewProjectionMatrix;
        case MATRIX::VIEW:
            return _gpuBlock._data._ViewMatrix;
        case MATRIX::PROJECTION:
            return _gpuBlock._data._ProjectionMatrix;
        case MATRIX::VIEW_INV:
            return _gpuBlock._viewMatrixInv;
        case MATRIX::PROJECTION_INV:
            return _gpuBlock._data._InvProjectionMatrix;
        case MATRIX::VIEW_PROJECTION_INV:
            return _gpuBlock._viewProjMatrixInv;
        case MATRIX::TEXTURE:
            Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_MATRIX_ACCESS")));
            break;
        default:
            DIVIDE_ASSERT(false, "GFXDevice error: attempted to query an invalid matrix target!");
            break;
    };

    return MAT4_IDENTITY;
}

/// Update the internal GPU data buffer with the clip plane values
void GFXDevice::updateClipPlanes() {
    GFXShaderData::GPUData& data = _gpuBlock._data;
    for (U8 i = 0; i < to_const_ubyte(Frustum::FrustPlane::COUNT); ++i) {
        data._clipPlanes[i].set(_clippingPlanes[i].getEquation());
    }
    _gpuBlock._needsUpload = true;
}

/// Update the internal GPU data buffer with the updated viewport dimensions
void GFXDevice::updateViewportInternal(const vec4<I32>& viewport) {
    // Change the viewport on the Rendering API level
    _api->changeViewport(viewport);
    // Update the buffer with the new value
    _gpuBlock._data._ViewPort.set(viewport.x, viewport.y, viewport.z, viewport.w);
    _gpuBlock._needsUpload = true;
}

void GFXDevice::updateViewportInternal(I32 x, I32 y, I32 width, I32 height) {
    updateViewportInternal(vec4<I32>(x, y, width, height));
}

void GFXDevice::setSceneZPlanes(const vec2<F32>& zPlanes) {
    GFXShaderData::GPUData& data = _gpuBlock._data;
    data._ZPlanesCombined.zw(zPlanes);
    _gpuBlock._needsUpload = true;
}

void GFXDevice::renderFromCamera(Camera& camera) {
    Camera::activeCamera(&camera);
    // Tell the Rendering API to draw from our desired PoV
    camera.updateLookAt();
    

    const mat4<F32>& viewMatrix = camera.getViewMatrix();
    const mat4<F32>& projMatrix = camera.getProjectionMatrix();

    GFXShaderData::GPUData& data = _gpuBlock._data;
    bool viewMatUpdated = viewMatrix != data._ViewMatrix;
    bool projMatUpdated = projMatrix != data._ProjectionMatrix;
    if (viewMatUpdated) {
        data._ViewMatrix.set(viewMatrix);
        data._ViewMatrix.getInverse(_gpuBlock._viewMatrixInv);
    }
    if (projMatUpdated) {
        data._ProjectionMatrix.set(projMatrix);
        data._ProjectionMatrix.getInverse(data._InvProjectionMatrix);
    }

    if (viewMatUpdated || projMatUpdated) {
        F32 FoV = camera.getVerticalFoV();
        data._cameraPosition.set(camera.getEye(), camera.getAspectRatio());
        data._renderProperties.zw(FoV, std::tan(FoV * 0.5f));
        data._ZPlanesCombined.xy(camera.getZPlanes());
        mat4<F32>::Multiply(data._ViewMatrix, data._ProjectionMatrix, data._ViewProjectionMatrix);
        data._ViewProjectionMatrix.getInverse(_gpuBlock._viewProjMatrixInv);
        Frustum::computePlanes(_gpuBlock._viewProjMatrixInv, data._frustumPlanes);
        _gpuBlock._needsUpload = true;
    }
}

/// Enable or disable 2D rendering mode 
/// (orthographic projection, no depth reads)
bool GFXDevice::toggle2D(bool state) {
    // Prevent double 2D toggle to the same state (e.g. in a loop)
    if (state == _2DRendering) {
        return false;
    }

    _2DRendering = state;
    // If we need to enable 2D rendering
    if (state) {
        // Upload 2D camera matrices to the GPU
        renderFromCamera(*_2DCamera);
    } else {
        Camera::activeCamera(Camera::previousCamera());
    }

    return true;
}

/// Update the rendering viewport
bool GFXDevice::setViewport(const vec4<I32>& viewport) {
    bool updated = false;
    // Avoid redundant changes
    if (!viewport.compare(_viewport.top())) {
        // Activate the new viewport
        updateViewportInternal(viewport);
        updated = true;
    }
    // Push the new viewport onto the stack
    _viewport.push(viewport);

    return updated;
}

/// Restore the viewport to it's previous value
bool GFXDevice::restoreViewport() {
    vec4<I32> crtViewport(_viewport.top());
    // Restore the viewport
    _viewport.pop();
    const vec4<I32>& prevViewport = _viewport.top();
    if (!crtViewport.compare(prevViewport)) {
        // Activate the new top viewport
        updateViewportInternal(prevViewport);
        return true;
    }

    return false;
}

/// Set a new viewport clearing the previous stack first
void GFXDevice::setBaseViewport(const vec4<I32>& viewport) {
    while (!_viewport.empty()) {
        _viewport.pop();
    }
    _viewport.push(viewport);

    // Set the new viewport
    updateViewportInternal(viewport);
}

void GFXDevice::onCameraUpdate(const Camera& camera) {
    ACKNOWLEDGE_UNUSED(camera);
}

void GFXDevice::onCameraChange(const Camera& camera) {
    ACKNOWLEDGE_UNUSED(camera);
}

/// Depending on the context, either immediately call the function, or pass it
/// to the loading thread via a queue
bool GFXDevice::loadInContext(const CurrentContext& context, const DELEGATE_CBK_PARAM<const Task&>& callback) {
    static const Task mainTask;
    // Skip invalid callbacks
    if (callback) {
        if (context == CurrentContext::GFX_LOADING_CTX && Config::USE_GPU_THREADED_LOADING) {
            CreateTask(callback)._task->startTask(Task::TaskPriority::HIGH, to_const_uint(Task::TaskFlags::SYNC_WITH_GPU));
        } else {
            if (Application::isMainThread()) {
                callback(mainTask);
            } else {
                WriteLock w_lock(_GFXLoadQueueLock);
                _GFXLoadQueue.push_back(callback);
            }
        }

        // The callback is valid and has been processed
        return true;
    }
    
    return false;
}

/// Transform our depth buffer to a HierarchicalZ buffer (for occlusion queries and screen space reflections)
/// Based on RasterGrid implementation: http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/
/// Modified with nVidia sample code: https://github.com/nvpro-samples/gl_occlusion_culling
void GFXDevice::constructHIZ(RenderTarget& depthBuffer) {
    static bool firstRun = true;
    static RTDrawDescriptor depthOnlyTarget;
    static GenericDrawCommand triangleCmd;
    
    if (firstRun) {
        // We use a special shader that downsamples the buffer
        // We will use a state block that disables colour writes as we will render only a depth image,
        // disables depth testing but allows depth writes
        // Set the depth buffer as the currently active render target
        depthOnlyTarget.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        depthOnlyTarget.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        depthOnlyTarget.disableState(RTDrawDescriptor::State::CHANGE_VIEWPORT);
        depthOnlyTarget.drawMask().disableAll();
        depthOnlyTarget.drawMask().setEnabled(RTAttachment::Type::Depth, 0, true);

        triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
        triangleCmd.drawCount(1);
        triangleCmd.stateHash(_stateDepthOnlyRenderingHash);
        triangleCmd.shaderProgram(_HIZConstructProgram);

        firstRun = false;
    }

    // The depth buffer's resolution should be equal to the screen's resolution
    RenderTarget& screenTarget = depthBuffer;
    U16 width = screenTarget.getWidth();
    U16 height = screenTarget.getHeight();
    U16 level = 0;
    U16 dim = width > height ? width : height;
    U16 twidth = width;
    U16 theight = height;
    bool wasEven = false;

    // Store the current width and height of each mip
    vec4<I32> previousViewport(_viewport.top());

    // Bind the depth texture to the first texture unit
    screenTarget.bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH), RTAttachment::Type::Depth, 0);
    screenTarget.begin(depthOnlyTarget);
    // We skip the first level as that's our full resolution image

    vec2<I32> depthInfo;
    while (dim) {
        if (level) {
            twidth = twidth < 1 ? 1 : twidth;
            theight = theight < 1 ? 1 : theight;
            // Update the viewport with the new resolution
            updateViewportInternal(0, 0, twidth, theight);
            // Bind next mip level for rendering but first restrict fetches only to previous level
            screenTarget.setMipLevel(level);
            depthInfo.set(level - 1, wasEven ? 1 : 0);
            _HIZConstructProgram->Uniform("depthInfo", depthInfo);
            // Dummy draw command as the full screen quad is generated completely in the vertex shader
            draw(triangleCmd);
        }

        // Calculate next viewport size
        wasEven = (twidth % 2 == 0) && (theight % 2 == 0);
        dim /= 2;
        twidth /= 2;
        theight /= 2;
        level++;
    }

    updateViewportInternal(previousViewport);
    // Reset mipmap level range for the depth buffer
    screenTarget.setMipLevel(0);
    // Unbind the render target
    screenTarget.end();
}

Renderer& GFXDevice::getRenderer() const {
    DIVIDE_ASSERT(_renderer != nullptr,
        "GFXDevice error: Renderer requested but not created!");
    return *_renderer;
}

void GFXDevice::setRenderer(RendererType rendererType) {
    DIVIDE_ASSERT(rendererType != RendererType::COUNT,
        "GFXDevice error: Tried to create an invalid renderer!");

    PlatformContext& context = parent().platformContext();
    ResourceCache& cache = parent().resourceCache();

    switch (rendererType) {
        case RendererType::RENDERER_TILED_FORWARD_SHADING: {
            MemoryManager::SAFE_UPDATE(_renderer, MemoryManager_NEW TiledForwardShadingRenderer(context, cache));
        } break;
        case RendererType::RENDERER_DEFERRED_SHADING: {
            MemoryManager::SAFE_UPDATE(_renderer, MemoryManager_NEW DeferredShadingRenderer(context, cache));
        } break;
    }
}

ShaderComputeQueue& GFXDevice::shaderComputeQueue() {
    assert(_shaderComputeQueue != nullptr);
    return *_shaderComputeQueue;
}

const ShaderComputeQueue& GFXDevice::shaderComputeQueue() const {
    assert(_shaderComputeQueue != nullptr);
    return *_shaderComputeQueue;
}

/// Extract the pixel data from the main render target's first colour attachment
/// and save it as a TGA image
void GFXDevice::Screenshot(const stringImpl& filename) {
    // Get the screen's resolution
    RenderTarget& screenRT = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
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
