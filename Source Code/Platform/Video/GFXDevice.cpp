#include "config.h"

#include "Headers/GFXDevice.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Managers/Headers/SceneManager.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"

#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

std::array<VertexBuffer::AttribFlags, to_const_uint(RenderStage::COUNT)> VertexBuffer::_attribMaskPerStage;

GFXDevice::GFXDevice()
    : _api(nullptr), 
    _renderStage(RenderStage::DISPLAY),
    _prevRenderStage(RenderStage::COUNT),
    _commandBuildTimer(Time::ADD_TIMER("Command Generation Timer"))
{
    // Hash values
    _state2DRenderingHash = 0;
    _defaultStateBlockHash = 0;
    _currentStateBlockHash = 0;
    _previousStateBlockHash = 0;
    _defaultStateNoDepthHash = 0;
    _stateDepthOnlyRenderingHash = 0;
    // Pointers
    _axisGizmo = nullptr;
    _imShader = nullptr;
    _gfxDataBuffer = nullptr;
    _HIZConstructProgram = nullptr;
    _HIZCullProgram = nullptr;
    _renderTargetDraw = nullptr;
    _previewDepthMapShader = nullptr;
    _displayShader = nullptr;
    _activeRenderTarget = nullptr;
    // Integers
    FRAME_COUNT = 0;
    FRAME_DRAW_CALLS = 0;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    _imShaderTextureFlag = -1;
    _imShaderWorldMatrix = -1;
    // Floats
    _interpolationFactor = 1.0;
    // Cameras
    _2DCamera = nullptr;
    _cubeCamera = nullptr;
    _dualParaboloidCamera = nullptr;
    // Booleans
    _2DRendering = false;
    _drawDebugAxis = false;
    _viewportUpdate = false;
    _rasterizationEnabled = true;
    _zWriteEnabled = true;
    // Enumerated Types
    _shadowDetailLevel = RenderDetailLevel::HIGH;
    _GPUVendor = GPUVendor::COUNT;
    _API_ID = RenderAPI::COUNT;
    // Clipping planes
    _clippingPlanes.resize(Config::MAX_CLIP_PLANES, Plane<F32>(0, 0, 0, 0));
    // Render targets
    for (RenderTargetWrapper& renderTarget : _renderTarget) {
        renderTarget._target = nullptr;
    }
    for (RenderTargetWrapper& renderTarget : _reflectionTarget) {
        renderTarget._target = nullptr;
    }
    for (RenderTargetWrapper& renderTarget : _refractionTarget) {
        renderTarget._target = nullptr;
    }
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
                                RenderStage renderStage) {
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

    Kernel& kernel = Application::instance().kernel();
    // Set a 90 degree vertical FoV perspective projection
    _cubeCamera->setProjection(1.0f, 90.0f, zPlanes);
    // Set the cube camera as the currently active one
    kernel.getCameraMgr().pushActiveCamera(_cubeCamera);
    // Set the desired render stage, remembering the previous one
    RenderStage prevRenderStage = setRenderStage(renderStage);
    // Enable our render target
    cubeMap.begin(RenderTarget::defaultPolicy());
    // For each of the environment's faces (TOP, DOWN, NORTH, SOUTH, EAST, WEST)
    for (U8 i = 0; i < 6; ++i) {
        // Draw to the current cubemap face
        cubeMap.drawToFace(hasColour ? RTAttachment::Type::Colour
                                     : RTAttachment::Type::Depth,
                           0,
                           i + arrayOffset);
        // Point our camera to the correct face
        _cubeCamera->lookAt(pos, TabCenter[i], TabUp[i]);
        // And generated required matrices
        _cubeCamera->renderLookAt();
        // Pass our render function to the renderer
        SceneManager::instance().renderVisibleNodes(renderStage, true, i);
    }
    // Resolve our render target
    cubeMap.end();
    // Return to our previous rendering stage
    setRenderStage(prevRenderStage);
    // Restore our previous camera
    kernel.getCameraMgr().popActiveCamera();
}

void GFXDevice::generateDualParaboloidMap(RenderTarget& targetBuffer,
                                          const U32 arrayOffset,
                                          const vec3<F32>& pos,
                                          const vec2<F32>& zPlanes,
                                          RenderStage renderStage)
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
    Kernel& kernel = Application::instance().kernel();
    // Set a 90 degree vertical FoV perspective projection
    _dualParaboloidCamera->setProjection(1.0f, 180.0f, zPlanes);
    // Set the cube camera as the currently active one
    kernel.getCameraMgr().pushActiveCamera(_dualParaboloidCamera);
    // Set the desired render stage, remembering the previous one
    RenderStage prevRenderStage = setRenderStage(renderStage);
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
            _dualParaboloidCamera->renderLookAt();
            // Pass our render function to the renderer
            SceneManager::instance().renderVisibleNodes(renderStage, true, i);
        }
    targetBuffer.end();
    // Return to our previous rendering stage
    setRenderStage(prevRenderStage);
    // Restore our previous camera
    kernel.getCameraMgr().popActiveCamera();
}

/// If the stateBlock doesn't exist in the state block map, add it for future reference
bool GFXDevice::registerRenderStateBlock(const RenderStateBlock& descriptor) {
    // Each combination of render states has a unique hash value
    size_t hashValue = descriptor.getHash();
    // Find the corresponding render state block
    // Create a new one if none are found. The GFXDevice class is
    // responsible for deleting these!
    std::pair<RenderStateMap::iterator, bool> result =
        hashAlg::emplace(_stateBlockMap, hashValue, descriptor);
    // Return true if registration was successful 
    return result.second;
}

/// Activate the render state block described by the specified hash value (0 == default state block)
size_t GFXDevice::setStateBlock(size_t stateBlockHash) {
    // Passing 0 is a perfectly acceptable way of enabling the default render state block
    if (stateBlockHash == 0) {
        stateBlockHash = _defaultStateBlockHash;
    }

    // If the new state hash is different from the previous one
    if (stateBlockHash != _currentStateBlockHash) {
        // Remember the previous state hash
        _previousStateBlockHash = _currentStateBlockHash;
        // Update the current state hash
        _currentStateBlockHash = stateBlockHash;
        RenderStateMap::const_iterator currentStateIt = _stateBlockMap.find(_currentStateBlockHash);
        RenderStateMap::const_iterator previousStateIt = _stateBlockMap.find(_previousStateBlockHash);

        DIVIDE_ASSERT(currentStateIt != previousStateIt &&
                      currentStateIt != std::cend(_stateBlockMap) &&
                      previousStateIt != std::cend(_stateBlockMap),
                      "GFXDevice error: Invalid state blocks detected on activation!");

        // Activate the new render state block in an rendering API dependent way
        _api->activateStateBlock(currentStateIt->second, previousStateIt->second);
    }
    // Return the previous state hash
    return _previousStateBlockHash;
}

/// Return the the render state block defined by the specified hash value.
const RenderStateBlock& GFXDevice::getRenderStateBlock(size_t renderStateBlockHash) const {
    // Find the render state block associated with the received hash value
    RenderStateMap::const_iterator it = _stateBlockMap.find(renderStateBlockHash);
    // Assert if it doesn't exist. Avoids programming errors.
    DIVIDE_ASSERT(it != std::cend(_stateBlockMap),
                  "GFXDevice error: Invalid render state block hash specified "
                  "for getRenderStateBlock!");
    // Return the state block's descriptor
    return it->second;
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
    // Make sure we are in a valid state that allows resolution updates
    if (_renderTarget[to_const_uint(RenderTargetID::SCREEN)]._target != nullptr) {
        // Update resolution only if it's different from the current one.
        // Avoid resolution change on minimize so we don't thrash render targets
        if ((w == _renderTarget[to_const_uint(RenderTargetID::SCREEN)]._target->getWidth() &&
             h == _renderTarget[to_const_uint(RenderTargetID::SCREEN)]._target->getHeight()) ||
           !(w > 1 && h > 1)) {
            return;
        }
        // Update render targets with the new resolution
        for (U32 i = 0; i < to_const_uint(RenderTargetID::COUNT); ++i) {
            RenderTarget* renderTarget = _renderTarget[i]._target;
            if (renderTarget) {
                renderTarget->create(w, h);
            }
        }
    }

    // Update post-processing render targets and buffers
    PostFX::instance().updateResolution(w, h);
    _gpuBlock._data._invScreenDimension.xy(1.0f / w, 1.0f / h);
    _gpuBlock._updated = true;
    // Update the 2D camera so it matches our new rendering viewport
    _2DCamera->setProjection(vec4<F32>(0, to_float(w), 0, to_float(h)), vec2<F32>(-1, 1));
}

/// Return a GFXDevice specific matrix or a derivative of it
void GFXDevice::getMatrix(const MATRIX& mode, mat4<F32>& mat) {
    // The matrix names are self-explanatory
    if (mode == MATRIX::VIEW_PROJECTION) {
        mat.set(_gpuBlock._data._ViewProjectionMatrix);
    } else if (mode == MATRIX::VIEW) {
        mat.set(_gpuBlock._data._ViewMatrix);
    } else if (mode == MATRIX::PROJECTION) {
        mat.set(_gpuBlock._data._ProjectionMatrix);
    } else if (mode == MATRIX::TEXTURE) {
        mat.identity();
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_MATRIX_ACCESS")));
    } else if (mode == MATRIX::VIEW_INV) {
        _gpuBlock._data._ViewMatrix.getInverse(mat);
    } else if (mode == MATRIX::PROJECTION_INV) {
        _gpuBlock._data._ProjectionMatrix.getInverse(mat);
    } else if (mode == MATRIX::VIEW_PROJECTION_INV) {
        _gpuBlock._data._ViewProjectionMatrix.getInverse(mat);
    } else {
        DIVIDE_ASSERT(
            false,
            "GFXDevice error: attempted to query an invalid matrix target!");
    }
}

/// Update the internal GPU data buffer with the clip plane values
void GFXDevice::updateClipPlanes() {
    GPUBlock::GPUData& data = _gpuBlock._data;
    for (U8 i = 0; i < Config::MAX_CLIP_PLANES; ++i) {
        data._clipPlanes[i] = _clippingPlanes[i].getEquation();
    }
    _gpuBlock._updated = true;
}

/// Update the internal GPU data buffer with the updated viewport dimensions
void GFXDevice::updateViewportInternal(const vec4<I32>& viewport) {
    // Change the viewport on the Rendering API level
    _api->changeViewport(viewport);
    // Update the buffer with the new value
    _gpuBlock._data._ViewPort.set(viewport.x, viewport.y, viewport.z, viewport.w);
    _gpuBlock._updated = true;
}

/// Update the virtual camera's matrices and upload them to the GPU
F32* GFXDevice::lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& eyePos) {
    bool updated = false;

    GPUBlock::GPUData& data = _gpuBlock._data;

    if (eyePos != _gpuBlock._data._cameraPosition.xyz()) {
        data._cameraPosition.xyz(eyePos);
        updated = true;
    }

    if (viewMatrix != _gpuBlock._data._ViewMatrix) {
        data._ViewMatrix.set(viewMatrix);
        updated = true;
    }

    if (updated) {
        mat4<F32>::Multiply(data._ViewMatrix, data._ProjectionMatrix, data._ViewProjectionMatrix);
        computeFrustumPlanes();
        _gpuBlock._updated = true;
    }

    return data._ViewMatrix.mat;
}

/// Enable an orthographic projection and upload the corresponding matrices to
/// the GPU
F32* GFXDevice::setProjection(const vec4<F32>& rect, const vec2<F32>& planes) {
    GPUBlock::GPUData& data = _gpuBlock._data;

    data._ProjectionMatrix.ortho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);

    data._ZPlanesCombined.xy(planes);

    mat4<F32>::Multiply(data._ViewMatrix, data._ProjectionMatrix, data._ViewProjectionMatrix);

    computeFrustumPlanes();

    _gpuBlock._updated = true;

    return data._ProjectionMatrix.mat;
}

/// Enable a perspective projection and upload the corresponding matrices to the
/// GPU
F32* GFXDevice::setProjection(F32 FoV, F32 aspectRatio,  const vec2<F32>& planes) {
    GPUBlock::GPUData& data = _gpuBlock._data;

    data._ProjectionMatrix.perspective(Angle::DegreesToRadians(FoV),
                                       aspectRatio,
                                       planes.x, planes.y);

    data._ZPlanesCombined.xy(planes);
    data._cameraPosition.w = aspectRatio;
    data._renderProperties.z = FoV;
    data._renderProperties.w = std::tan(FoV * 0.5f);
    mat4<F32>::Multiply(data._ViewMatrix, data._ProjectionMatrix, data._ViewProjectionMatrix);
    computeFrustumPlanes();
    _gpuBlock._updated = true;

    return data._ProjectionMatrix.mat;
}

void GFXDevice::computeFrustumPlanes(const mat4<F32>& invViewProj, vec4<F32>* planesOut) {
    // Get world-space coordinates for clip-space bounds.
    vec4<F32> lbn(invViewProj * vec4<F32>(-1, -1, -1, 1));
    vec4<F32> ltn(invViewProj * vec4<F32>(-1, 1, -1, 1));
    vec4<F32> lbf(invViewProj * vec4<F32>(-1, -1, 1, 1));
    vec4<F32> rbn(invViewProj * vec4<F32>(1, -1, -1, 1));
    vec4<F32> rtn(invViewProj * vec4<F32>(1, 1, -1, 1));
    vec4<F32> rbf(invViewProj * vec4<F32>(1, -1, 1, 1));
    vec4<F32> rtf(invViewProj * vec4<F32>(1, 1, 1, 1));

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

    planesOut[0].set(left_normal, -Dot(left_normal, lbn_pos)); // Left
    planesOut[1].set(right_normal, -Dot(right_normal, rbn_pos)); // Right
    planesOut[2].set(near_normal, -Dot(near_normal, lbn_pos)); // Near
    planesOut[3].set(far_normal, -Dot(far_normal, lbf_pos)); // Far
    planesOut[4].set(top_normal, -Dot(top_normal, ltn_pos)); // Top
    planesOut[5].set(bottom_normal, -Dot(bottom_normal, lbn_pos)); // Bottom
}

void GFXDevice::computeFrustumPlanes() {
    computeFrustumPlanes(getMatrix(MATRIX::VIEW_PROJECTION_INV), _gpuBlock._data._frustumPlanes);
    _gpuBlock._updated = true;
}

/// Enable or disable 2D rendering mode 
/// (orthographic projection, no depth reads)
void GFXDevice::toggle2D(bool state) {
    // Remember the previous state hash
    static size_t previousStateBlockHash = 0;
    // Prevent double 2D toggle to the same state (e.g. in a loop)
    if (state == _2DRendering) {
        return;
    }
    Kernel& kernel = Application::instance().kernel();
    _2DRendering = state;
    // If we need to enable 2D rendering
    if (state) {
        // Activate the 2D render state block
        previousStateBlockHash = setStateBlock(_state2DRenderingHash);
        // Push the 2D camera
        kernel.getCameraMgr().pushActiveCamera(_2DCamera);
        // Upload 2D camera matrices to the GPU
        _2DCamera->renderLookAt();
    } else {
        // Reverting to 3D implies popping the 2D camera
        kernel.getCameraMgr().popActiveCamera();
        // And restoring the previous state block
        setStateBlock(previousStateBlockHash);
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
    if (drawDebugAxis()) {
        // We need to transform the gizmo so that it always remains axis aligned
        // Create a world matrix using a look at function with the eye position
        // backed up from the camera's view direction
        _axisGizmo->worldMatrix(
            mat4<F32>(-camera.getViewDir() * 2, VECTOR3_ZERO, camera.getUpDir()) *
            _gpuBlock._data._ViewMatrix.getInverse());
        _axisGizmo->paused(false);
    } else {
        _axisGizmo->paused(true);
    }
}

/// Depending on the context, either immediately call the function, or pass it
/// to the loading thread via a queue
bool GFXDevice::loadInContext(const CurrentContext& context, const DELEGATE_CBK_PARAM<bool>& callback) {
    // Skip invalid callbacks
    if (!callback) {
        return false;
    }

    if (context == CurrentContext::GFX_LOADING_CTX && Config::USE_GPU_THREADED_LOADING) {
        CreateTask(callback)._task->startTask(Task::TaskPriority::HIGH,
                                              to_const_uint(Task::TaskFlags::SYNC_WITH_GPU));
    } else {
        callback(false);
    }

    // The callback is valid and has been processed
    return true;
}

/// Transform our depth buffer to a HierarchicalZ buffer (for occlusion queries and screen space reflections)
void GFXDevice::constructHIZ() {
    static bool firstRun = true;
    static RTDrawDescriptor depthOnlyTarget;

    if (firstRun) {
        // We use a special shader that downsamples the buffer
        // We will use a state block that disables colour writes as we will render only a depth image,
        // disables depth testing but allows depth writes
        // Set the depth buffer as the currently active render target
        depthOnlyTarget._clearColourBuffersOnBind = false;
        depthOnlyTarget._clearDepthBufferOnBind = false;
        depthOnlyTarget._changeViewport = false;
        depthOnlyTarget._drawMask.disableAll();
        depthOnlyTarget._drawMask.enabled(RTAttachment::Type::Depth, 0);

        firstRun = false;
    }

    auto setAndGetHalfViewport = [](vec4<I32>& viewportIn) -> vec4<I32>& {
        viewportIn /= 2;
        // Ensure that the viewport size is always at least 1x1
        viewportIn.z = viewportIn.z > 0 ? viewportIn.z : 1;
        viewportIn.w = viewportIn.w > 0 ? viewportIn.w : 1;
        return viewportIn;
    };

    // The depth buffer's resolution should be equal to the screen's resolution
    RenderTarget* screenTarget = _renderTarget[to_const_uint(RenderTargetID::SCREEN)]._target;
    U16 width = screenTarget->getWidth();
    U16 height = screenTarget->getHeight();
    // Bind the depth texture to the first texture unit
    screenTarget->bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH), RTAttachment::Type::Depth, 0);


    screenTarget->begin(depthOnlyTarget);
    // Calculate the number of mipmap levels we need to generate
    U32 numLevels = 1 + to_uint(floorf(log2f(fmaxf(to_float(width),
                                                   to_float(height)))));
    // Store the current width and height of each mip
    vec4<I32> currentViewport(0, 0, width, height);
    vec4<I32> previousViewport(_viewport.top());

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(_stateDepthOnlyRenderingHash);
    triangleCmd.shaderProgram(_HIZConstructProgram);

    // We skip the first level as that's our full resolution image
    for (U16 i = 1; i < numLevels; ++i) {
        // Calculate next viewport size
        // Update the viewport with the new resolution
        updateViewportInternal(setAndGetHalfViewport(currentViewport));
        // Bind next mip level for rendering but first restrict fetches only to previous level
        screenTarget->setMipLevel(i - 1, i - 1, i, RTAttachment::Type::Depth, 0);
        // Dummy draw command as the full screen quad is generated completely in the vertex shader
        draw(triangleCmd);
    }

    updateViewportInternal(previousViewport);
    // Reset mipmap level range for the depth buffer
    screenTarget->resetMipLevel(RTAttachment::Type::Depth, 0);
    // Unbind the render target
    screenTarget->end();
}

/// Find an unused primitive object or create a new one and return it
IMPrimitive* GFXDevice::getOrCreatePrimitive(bool allowPrimitiveRecycle) {
    UpgradableReadLock ur_lock(_imInterfaceLock);
    IMPrimitive* tempPriv = nullptr;
    // Find an available and unused primitive (a zombie primitive)
    vectorImpl<IMPrimitive*>::iterator it;
    it = std::find_if(std::begin(_imInterfaces), std::end(_imInterfaces),
                      [](IMPrimitive* const priv) { 
                            return (priv && !priv->inUse()); 
                      });
    // If we allow resurrected primitives check if we have one available
    if (allowPrimitiveRecycle && it != std::end(_imInterfaces)) {
        tempPriv = *it;
        // If we have a valid zombie, resurrect it
        tempPriv->clear();

    } else {
        // If we do not have a valid zombie, we create a new primitive
        tempPriv = _api->newIMP(*this);
        // And add it to our container. The GFXDevice class is responsible for deleting these!
        UpgradeToWriteLock w_lock(ur_lock);
        _imInterfaces.push_back(tempPriv);
    }
    // Toggle zombification of the primitive on or off depending on our request
    tempPriv->_canZombify = allowPrimitiveRecycle;

    return tempPriv;
}

/// Extract the pixel data from the main render target's first colour attachment
/// and save it as a TGA image
void GFXDevice::Screenshot(const stringImpl& filename) {
    // Get the screen's resolution
    U16 width = _renderTarget[to_const_uint(RenderTargetID::SCREEN)]._target->getWidth();
    U16 height = _renderTarget[to_const_uint(RenderTargetID::SCREEN)]._target->getHeight();
    // Allocate sufficiently large buffers to hold the pixel data
    U32 bufferSize = width * height * 4;
    U8* imageData = MemoryManager_NEW U8[bufferSize];
    // Read the pixels from the main render target (RGBA16F)
    _renderTarget[to_const_uint(RenderTargetID::SCREEN)]._target->readData(
        GFXImageFormat::RGBA, GFXDataFormat::UNSIGNED_BYTE, imageData);
    // Save to file
    ImageTools::SaveSeries(filename,
                           vec2<U16>(width, height),
                           32,
                           imageData);
    // Delete local buffers
    MemoryManager::DELETE_ARRAY(imageData);
}
};
