#include "config.h"

#include "Headers/GFXDevice.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

GFXDevice::GFXDevice()
    : _api(nullptr), 
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
    _activeRenderTarget = nullptr;
    _debugFrustum = nullptr;
    _debugFrustumPrimitive = nullptr;
    // Integers
    FRAME_COUNT = 0;
    FRAME_DRAW_CALLS = 0;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    _graphicResources = 0;
    // Floats
    _interpolationFactor = 1.0;
    // Cameras
    _2DCamera = nullptr;
    _cubeCamera = nullptr;
    _dualParaboloidCamera = nullptr;
    // Booleans
    _2DRendering = false;
    _viewportUpdate = false;
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
                                const U32 arrayOffset,
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

    RenderPassManager& passMgr = RenderPassManager::instance();
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
                                          const U32 arrayOffset,
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

    RenderPassManager& passMgr = RenderPassManager::instance();
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
    for (RenderTarget* rt : _rtPool.renderTargets(RenderTargetUsage::SCREEN)) {
        if (rt) {
            rt->create(w, h);
        }
    }

    _previousDepthBuffer._rt->create(w, h);

    // Update post-processing render targets and buffers
    PostFX::instance().updateResolution(w, h);
    _gpuBlock._data._invScreenDimension.xy(1.0f / w, 1.0f / h);
    _gpuBlock._needsUpload = true;
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
    GPUBlock::GPUData& data = _gpuBlock._data;
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

void GFXDevice::renderFromCamera(Camera& camera) {
    Camera::activeCamera(&camera);

    camera.updateLookAt();
    // Tell the Rendering API to draw from our desired PoV
    GPUBlock::GPUData& data = _gpuBlock._data;

    const mat4<F32>& viewMatrix = camera.getViewMatrix();
    const mat4<F32>& projMatrix = camera.getProjectionMatrix();

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
        GFXDevice::computeFrustumPlanes(_gpuBlock._viewProjMatrixInv, data._frustumPlanes);
        _gpuBlock._needsUpload = true;
    }
}

/// Enable or disable 2D rendering mode 
/// (orthographic projection, no depth reads)
void GFXDevice::toggle2D(bool state) {
    // Prevent double 2D toggle to the same state (e.g. in a loop)
    if (state == _2DRendering) {
        return;
    }

    _2DRendering = state;
    // If we need to enable 2D rendering
    if (state) {
        // Upload 2D camera matrices to the GPU
        renderFromCamera(*_2DCamera);
    } else {
        Camera::activeCamera(Camera::previousCamera());
    }
}

/// Update the rendering viewport
void GFXDevice::setViewport(const vec4<I32>& viewport) {
    // Avoid redundant changes
    _viewportUpdate = !viewport.compare(_viewport.top());

    if (_viewportUpdate) {
        // Push the new viewport
        _viewport.push(viewport);
        // Activate the new viewport
        updateViewportInternal(viewport);
    }
}

/// Restore the viewport to it's previous value
void GFXDevice::restoreViewport() {
    // If we didn't push a new viewport, there's nothing to pop
    if (!_viewportUpdate) {
        return;
    }
    // Restore the viewport
    _viewport.pop();
    // Activate the new top viewport
    updateViewportInternal(_viewport.top());
    _viewportUpdate = false;
}

/// Set a new viewport clearing the previous stack first
void GFXDevice::setBaseViewport(const vec4<I32>& viewport) {
    while (!_viewport.empty()) {
        _viewport.pop();
    }
    _viewport.push(viewport);

    // Set the new viewport
    updateViewportInternal(viewport);
    // The forced viewport can't be popped
    _viewportUpdate = false;
}

void GFXDevice::onCameraUpdate(Camera& camera) {
    ACKNOWLEDGE_UNUSED(camera);
}

void GFXDevice::onCameraChange(Camera& camera) {
    ACKNOWLEDGE_UNUSED(camera);
}

/// Depending on the context, either immediately call the function, or pass it
/// to the loading thread via a queue
bool GFXDevice::loadInContext(const CurrentContext& context, const DELEGATE_CBK_PARAM<bool>& callback) {
    // Skip invalid callbacks
    if (callback) {
        if (context == CurrentContext::GFX_LOADING_CTX && Config::USE_GPU_THREADED_LOADING) {
            CreateTask(callback)._task->startTask(Task::TaskPriority::HIGH, to_const_uint(Task::TaskFlags::SYNC_WITH_GPU));
        } else {
            if (Application::isMainThread()) {
                callback(false);
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

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(_stateDepthOnlyRenderingHash);
    triangleCmd.shaderProgram(_HIZConstructProgram);

    // Bind the depth texture to the first texture unit
    screenTarget.bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH), RTAttachment::Type::Depth, 0);
    screenTarget.begin(depthOnlyTarget);
    // We skip the first level as that's our full resolution image
    while (dim) {
        if (level) {
            twidth = twidth < 1 ? 1 : twidth;
            theight = theight < 1 ? 1 : theight;
            // Update the viewport with the new resolution
            updateViewportInternal(0, 0, twidth, theight);
            // Bind next mip level for rendering but first restrict fetches only to previous level
            screenTarget.setMipLevel(level, RTAttachment::Type::Depth, 0);
            _HIZConstructProgram->Uniform("depthLoD", level-1);
            _HIZConstructProgram->Uniform("isDepthEven", wasEven);
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
    screenTarget.resetMipLevel(RTAttachment::Type::Depth, 0);
    // Unbind the render target
    screenTarget.end();
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

void GFXDevice::computeFrustumPlanes(const mat4<F32>& invViewProj, Plane<F32>* planesOut) {
    std::array<vec4<F32>, to_const_uint(Frustum::FrustPlane::COUNT)> planesTemp;

    computeFrustumPlanes(invViewProj, planesTemp.data());
    for (U8 i = 0; i < to_ubyte(Frustum::FrustPoints::COUNT); ++i) {
        planesOut[i].set(planesTemp[i]);
    }
}

void GFXDevice::computeFrustumPlanes(const mat4<F32>& invViewProj, vec4<F32>* planesOut) {
    static const vec4<F32> unitVecs[] = { vec4<F32>(-1, -1, -1, 1),
                                          vec4<F32>(-1 , 1, -1, 1),
                                          vec4<F32>(-1, -1,  1, 1),
                                          vec4<F32>( 1, -1, -1, 1),
                                          vec4<F32>( 1,  1, -1, 1),
                                          vec4<F32>( 1, -1,  1, 1),
                                          vec4<F32>( 1,  1,  1, 1) };

    // Get world-space coordinates for clip-space bounds.
    vec4<F32> lbn(invViewProj * unitVecs[0]);
    vec4<F32> ltn(invViewProj * unitVecs[1]);
    vec4<F32> lbf(invViewProj * unitVecs[2]);
    vec4<F32> rbn(invViewProj * unitVecs[3]);
    vec4<F32> rtn(invViewProj * unitVecs[4]);
    vec4<F32> rbf(invViewProj * unitVecs[5]);
    vec4<F32> rtf(invViewProj * unitVecs[6]);

    vec3<F32> lbn_pos(lbn.xyz() / lbn.w);
    vec3<F32> ltn_pos(ltn.xyz() / ltn.w);
    vec3<F32> lbf_pos(lbf.xyz() / lbf.w);
    vec3<F32> rbn_pos(rbn.xyz() / rbn.w);
    vec3<F32> rtn_pos(rtn.xyz() / rtn.w);
    vec3<F32> rbf_pos(rbf.xyz() / rbf.w);
    vec3<F32> rtf_pos(rtf.xyz() / rtf.w);

    // Get plane equations for all sides of frustum.
    vec3<F32> left_normal(Cross(lbf_pos - lbn_pos, ltn_pos - lbn_pos));
    left_normal.normalize();
    vec3<F32> right_normal(Cross(rtn_pos - rbn_pos, rbf_pos - rbn_pos));
    right_normal.normalize();
    vec3<F32> top_normal(Cross(ltn_pos - rtn_pos, rtf_pos - rtn_pos));
    top_normal.normalize();
    vec3<F32> bottom_normal(Cross(rbf_pos - rbn_pos, lbn_pos - rbn_pos));
    bottom_normal.normalize();
    vec3<F32> near_normal(Cross(ltn_pos - lbn_pos, rbn_pos - lbn_pos));
    near_normal.normalize();
    vec3<F32> far_normal(Cross(rtf_pos - rbf_pos, lbf_pos - rbf_pos));
    far_normal.normalize();

    planesOut[to_const_uint(Frustum::FrustPlane::PLANE_LEFT)].set(  left_normal,   -Dot(left_normal, lbn_pos));
    planesOut[to_const_uint(Frustum::FrustPlane::PLANE_RIGHT)].set( right_normal,  -Dot(right_normal, rbn_pos));
    planesOut[to_const_uint(Frustum::FrustPlane::PLANE_NEAR)].set(  near_normal,   -Dot(near_normal, lbn_pos));
    planesOut[to_const_uint(Frustum::FrustPlane::PLANE_FAR)].set(   far_normal,    -Dot(far_normal, lbf_pos));
    planesOut[to_const_uint(Frustum::FrustPlane::PLANE_TOP)].set(   top_normal,    -Dot(top_normal, ltn_pos));
    planesOut[to_const_uint(Frustum::FrustPlane::PLANE_BOTTOM)].set(bottom_normal, -Dot(bottom_normal, lbn_pos));
}
};
