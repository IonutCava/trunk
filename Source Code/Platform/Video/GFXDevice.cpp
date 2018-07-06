#include "Headers/GFXDevice.h"

#include "Core/Headers/ParamHandler.h"
#include "Utility/Headers/ImageTools.h"
#include "Managers/Headers/SceneManager.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"

#include "Platform/Video/Shaders/Headers/ShaderManager.h"

namespace Divide {

namespace {
/// Used for anaglyph rendering
struct CameraFrustum {
    D32 leftfrustum;
    D32 rightfrustum;
    D32 bottomfrustum;
    D32 topfrustum;
    F32 modeltranslation;
} _leftCam, _rightCam;
F32 _anaglyphIOD = -0.01f;
};

GFXDevice::GFXDevice()
    : _api(nullptr), 
    _renderStage(RenderStage::COUNT),
    _prevRenderStage(RenderStage::COUNT)
{
    // Hash values
    _state2DRenderingHash = 0;
    _defaultStateBlockHash = 0;
    _currentStateBlockHash = 0;
    _previousStateBlockHash = 0;
    _defaultStateNoDepthHash = 0;
    _stateDepthOnlyRenderingHash = 0;
    // Pointers
    _imShader = nullptr;
    _nodeBuffer = nullptr;
    _gfxDataBuffer = nullptr;
    _HIZConstructProgram = nullptr;
    _previewDepthMapShader = nullptr;
    _commandBuildTimer = nullptr;
    // Integers
    _lastNodeCount = 0;
    FRAME_COUNT = 0;
    FRAME_DRAW_CALLS = 0;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    // Floats
    _interpolationFactor = 1.0;
    // Booleans
    _2DRendering = false;
    _drawDebugAxis = false;
    _enableAnaglyph = false;
    _viewportUpdate = false;
    _rasterizationEnabled = true;
    _enablePostProcessing = false;
    // Enumerated Types
    _shadowDetailLevel = RenderDetailLevel::HIGH;
    _GPUVendor = GPUVendor::COUNT;
    _API_ID = RenderAPI::COUNT;
    // Utility cameras
    _2DCamera = MemoryManager_NEW FreeFlyCamera();
    _2DCamera->lockView(true);
    _cubeCamera = MemoryManager_NEW FreeFlyCamera();
    // Clipping planes
    _clippingPlanes.resize(Config::MAX_CLIP_PLANES, Plane<F32>(0, 0, 0, 0));
    // Render targets
    for (Framebuffer*& renderTarget : _renderTarget) {
        renderTarget = nullptr;
    }
    // To allow calls to "setBaseViewport"
    _viewport.push(vec4<I32>(-1));
    // Add our needed app-wide render passes. RenderPassManager is responsible
    // for deleting these!
    RenderPassManager::getInstance().addRenderPass("diffusePass", 1);
    // RenderPassManager::getInstance().addRenderPass("shadowPass",2);
    // Red X-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_X_AXIS * 2, vec4<U8>(255, 0, 0, 255)));
    // Green Y-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Y_AXIS * 2, vec4<U8>(0, 255, 0, 255)));
    // Blue Z-axis
    _axisLines.push_back(
        Line(VECTOR3_ZERO, WORLD_Z_AXIS * 2, vec4<U8>(0, 0, 255, 255)));
}

GFXDevice::~GFXDevice()
{
}

/// Generate a cube texture and store it in the provided framebuffer
void GFXDevice::generateCubeMap(Framebuffer& cubeMap, const vec3<F32>& pos,
                                const DELEGATE_CBK<>& renderFunction,
                                const vec2<F32>& zPlanes,
                                RenderStage renderStage) {
    // Only the first color attachment or the depth attachment is used for now
    // and it must be a cube map texture
    Texture* colorAttachment =
        cubeMap.GetAttachment(TextureDescriptor::AttachmentType::Color0);
    Texture* depthAttachment =
        cubeMap.GetAttachment(TextureDescriptor::AttachmentType::Depth);
    // Color attachment takes precedent over depth attachment
    bool hasColor = (colorAttachment != nullptr);
    bool hasDepth = (depthAttachment != nullptr);
    // Everyone's innocent until prove guilty
    bool isValidFB = true;
    if (hasColor) {
        // We only need the color attachment
        isValidFB = (colorAttachment->getTextureType() ==
                     TextureType::TEXTURE_CUBE_MAP);
    } else {
        // We don't have a color attachment, so we require a cube map depth
        // attachment
        isValidFB = (hasDepth &&
                     depthAttachment->getTextureType() ==
                         TextureType::TEXTURE_CUBE_MAP);
    }
    // Make sure we have a proper render target to draw to
    if (!isValidFB) {
        // Future formats must be added later (e.g. cube map arrays)
        Console::errorfn(Locale::get("ERROR_GFX_DEVICE_INVALID_FB_CUBEMAP"));
        return;
    }
    // Calling this function without a callback is a programming error and
    // should never happen
    DIVIDE_ASSERT(renderFunction == false,
                  "GFXDevice error: tried to generate a cube map without a "
                  "valid render function!");
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

    Kernel& kernel = Application::getInstance().getKernel();
    // Set a 90 degree vertical FoV perspective projection
    _cubeCamera->setProjection(1.0f, 90.0f, zPlanes);
    // Set the cube camera as the currently active one
    kernel.getCameraMgr().pushActiveCamera(_cubeCamera, false);
    // Set the desired render stage, remembering the previous one
    RenderStage prevRenderStage = setRenderStage(renderStage);
    // Enable our render target
    cubeMap.Begin(Framebuffer::defaultPolicy());
    // For each of the environment's faces (TOP, DOWN, NORTH, SOUTH, EAST, WEST)
    for (U8 i = 0; i < 6; ++i) {
        // Draw to the current cubemap face
        cubeMap.DrawToFace(hasColor ? TextureDescriptor::AttachmentType::Color0
                                    : TextureDescriptor::AttachmentType::Depth,
                           i);
        // Point our camera to the correct face
        _cubeCamera->lookAt(pos, TabCenter[i], TabUp[i]);
        // And generated required matrices
        _cubeCamera->renderLookAt();
        // Pass our render function to the renderer
        getRenderer().render(renderFunction, GET_ACTIVE_SCENE().renderState());
    }
    // Resolve our render target
    cubeMap.End();
    // Return to our previous rendering stage
    setRenderStage(prevRenderStage);
    // Restore our previous camera
    kernel.getCameraMgr().popActiveCamera();
}

/// If the stateBlock doesn't exist in the state block map, add it for future reference
bool GFXDevice::registerRenderStateBlock(
    const RenderStateBlock& descriptor) {
    // Each combination of render states has a unique hash value
    size_t hashValue = descriptor.getHash();
    // Find the corresponding render state block
    // Create a new one if none are found. The GFXDevice class is
    // responsible for deleting these!
    std::pair<RenderStateMap::iterator, bool> result =
        hashAlg::emplace(_stateBlockMap, hashValue,
                         MemoryManager_NEW RenderStateBlock(descriptor));
    // Return true if registration was successful 
    return result.second;
}

/// Activate the render state block described by the specified hash value (0 ==
/// default state block)
size_t GFXDevice::setStateBlock(size_t stateBlockHash) {
    // Passing 0 is a perfectly acceptable way of enabling the default render
    // state block
    stateBlockHash =
        stateBlockHash == 0 ? _defaultStateBlockHash : stateBlockHash;
    // If the new state hash is different from the previous one
    if (_currentStateBlockHash == 0 ||
        stateBlockHash != _currentStateBlockHash) {
        // Remember the previous state hash
        _previousStateBlockHash = _currentStateBlockHash;
        // Update the current state hash
        _currentStateBlockHash = stateBlockHash;
        RenderStateMap::const_iterator currentStateIt =
            _stateBlockMap.find(_currentStateBlockHash);
        RenderStateMap::const_iterator previousStateIt =
            _stateBlockMap.find(_previousStateBlockHash);
        DIVIDE_ASSERT(
            currentStateIt != std::end(_stateBlockMap) &&
                previousStateIt != std::end(_stateBlockMap),
            "GFXDevice error: Invalid state blocks detected on activation!");
        // Activate the new render state block in an rendering API dependent way
        activateStateBlock(*currentStateIt->second, previousStateIt->second);
    }
    // Return the previous state hash
    return _previousStateBlockHash;
}

/// Return the the render state block defined by the specified hash value.
const RenderStateBlock& GFXDevice::getRenderStateBlock(size_t renderStateBlockHash) const {
    // Find the render state block associated with the received hash value
    RenderStateMap::const_iterator it = _stateBlockMap.find(renderStateBlockHash);
    // Assert if it doesn't exist. Avoids programming errors.
    DIVIDE_ASSERT(it != std::end(_stateBlockMap),
                  "GFXDevice error: Invalid render state block hash specified "
                  "for getRenderStateBlock!");
    // Return the state block's descriptor
    return *it->second;
}

void GFXDevice::increaseResolution() {
    const WindowManager& winManager = Application::getInstance().getWindowManager();
    const vec2<U16>& resolution = winManager.getResolution();
    const vectorImpl<GPUState::GPUVideoMode>& displayModes = _state.getDisplayModes(winManager.targetDisplay());

    vectorImpl<GPUState::GPUVideoMode>::const_reverse_iterator it;
    for (it = std::rbegin(displayModes); it != std::rend(displayModes); ++it) {
        const vec2<U16>& tempResolution = it->_resolution;

        if (resolution.width < tempResolution.width &&
            resolution.height < tempResolution.height) {
            changeResolution(tempResolution.width, tempResolution.height);
            return;
        }
    }
}

void GFXDevice::decreaseResolution() {
    const WindowManager& winManager = Application::getInstance().getWindowManager();
    const vec2<U16>& resolution = winManager.getResolution();
    const vectorImpl<GPUState::GPUVideoMode>& displayModes = _state.getDisplayModes(winManager.targetDisplay());
    
    vectorImpl<GPUState::GPUVideoMode>::const_iterator it;
    for (it = std::begin(displayModes); it != std::end(displayModes); ++it) {
        const vec2<U16>& tempResolution = it->_resolution;
        if (resolution.width > tempResolution.width &&
            resolution.height > tempResolution.height) {
            changeResolution(tempResolution.width, tempResolution.height);
            return;
        }
    }
}

void GFXDevice::toggleFullScreen() {
    WindowManager& winManager = Application::getInstance().getWindowManager();
    switch (winManager.mainWindowType()) {
        case WindowType::WINDOW:
        case WindowType::SPLASH:
            winManager.mainWindowType(WindowType::FULLSCREEN_WINDOWED);
            break;
        case WindowType::FULLSCREEN_WINDOWED:
            winManager.mainWindowType(WindowType::FULLSCREEN);
            break;
        case WindowType::FULLSCREEN:
            winManager.mainWindowType(WindowType::WINDOW);
            break;
    };
}

/// The main entry point for any resolution change request
void GFXDevice::changeResolution(U16 w, U16 h) {
    // Make sure we are in a valid state that allows resolution updates
    if (_renderTarget[to_uint(RenderTarget::SCREEN)] != nullptr) {
        // Update resolution only if it's different from the current one.
        // Avoid resolution change on minimize so we don't thrash render targets
        if (vec2<U16>(w, h) ==  _renderTarget[to_uint(RenderTarget::SCREEN)]
                    ->getResolution() ||
            !(w > 1 && h > 1)) {
            return;
        }
        // Update render targets with the new resolution
        for (Framebuffer* renderTarget : _renderTarget) {
            if (renderTarget) {
                renderTarget->Create(w, h);
            }
        }
    }

    Application& app = Application::getInstance();
    // Update post-processing render targets and buffers
    PostFX::getInstance().updateResolution(w, h);
    app.getWindowManager().setResolution(vec2<U16>(w, h));
    // Refresh shader programs
    ShaderManager::getInstance().refreshShaderData();

    _api->changeResolution(w, h);
}

/// Update fog values
void GFXDevice::enableFog(F32 density, const vec3<F32>& color) {
    ParamHandler& par = ParamHandler::getInstance();
    par.setParam("rendering.sceneState.fogColor.r", color.r);
    par.setParam("rendering.sceneState.fogColor.g", color.g);
    par.setParam("rendering.sceneState.fogColor.b", color.b);
    par.setParam("rendering.sceneState.fogDensity", density);
    // Shader programs will pick up the new values on the next update call
    ShaderManager::getInstance().refreshSceneData();
}

/// Return a GFXDevice specific matrix or a derivative of it
void GFXDevice::getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat) {
    // The matrix names are self-explanatory
    if (mode == MATRIX_MODE::VIEW_PROJECTION) {
        mat.set(_gpuBlock._data._ViewProjectionMatrix);
    } else if (mode == MATRIX_MODE::VIEW) {
        mat.set(_gpuBlock._data._ViewMatrix);
    } else if (mode == MATRIX_MODE::PROJECTION) {
        mat.set(_gpuBlock._data._ProjectionMatrix);
    } else if (mode == MATRIX_MODE::TEXTURE) {
        mat.identity();
        Console::errorfn(Locale::get("ERROR_TEXTURE_MATRIX_ACCESS"));
    } else if (mode == MATRIX_MODE::VIEW_INV) {
        _gpuBlock._data._ViewMatrix.getInverse(mat);
    } else if (mode == MATRIX_MODE::PROJECTION_INV) {
        _gpuBlock._data._ProjectionMatrix.getInverse(mat);
    } else if (mode == MATRIX_MODE::VIEW_PROJECTION_INV) {
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
    changeViewport(viewport);
    // Update the buffer with the new value
    _gpuBlock._data._ViewPort.set(viewport.x, viewport.y, viewport.z, viewport.w);
    _gpuBlock._updated = true;
}

/// Update the virtual camera's matrices and upload them to the GPU
F32* GFXDevice::lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& eyePos) {
    bool updated = false;

    GPUBlock::GPUData& data = _gpuBlock._data;

    if (eyePos != _gpuBlock._data._cameraPosition) {
        data._cameraPosition.set(eyePos);
        updated = true;
    }

    if (viewMatrix != _gpuBlock._data._ViewMatrix) {
        data._ViewMatrix.set(viewMatrix);
        updated = true;
    }

    if (updated) {
        data._ViewProjectionMatrix.set(data._ViewMatrix * data._ProjectionMatrix);
        _gpuBlock._updated = true;
    }

    return data._ViewMatrix.mat;
}

/// Enable an orthographic projection and upload the corresponding matrices to
/// the GPU
F32* GFXDevice::setProjection(const vec4<F32>& rect, const vec2<F32>& planes) {
    GPUBlock::GPUData& data = _gpuBlock._data;

    data._ProjectionMatrix.ortho(rect.x, rect.y, rect.z, rect.w,
                                 planes.x, planes.y);

    data._ZPlanesCombined.xy(planes);

    data._ViewProjectionMatrix.set(data._ViewMatrix * data._ProjectionMatrix);

    _gpuBlock._updated = true;

    return data._ProjectionMatrix.mat;
}

/// Enable a perspective projection and upload the corresponding matrices to the
/// GPU
F32* GFXDevice::setProjection(F32 FoV, F32 aspectRatio,
                              const vec2<F32>& planes) {
    GPUBlock::GPUData& data = _gpuBlock._data;

    data._ProjectionMatrix.perspective(Angle::DegreesToRadians(FoV),
                                       aspectRatio,
                                       planes.x, planes.y);

    data._ZPlanesCombined.xy(planes);

    data._ViewProjectionMatrix.set(data._ViewMatrix * data._ProjectionMatrix);

    _gpuBlock._updated = true;

    return data._ProjectionMatrix.mat;
}

/// Calculate a frustum for the requested eye (left-right frustum) for anaglyph
/// rendering
void GFXDevice::setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes,
                                   F32 aspectRatio, F32 verticalFoV,
                                   bool rightFrustum) {
    // Only update frustum values if the interocular distance changed from the
    // previous request
    if (!FLOAT_COMPARE(_anaglyphIOD, camIOD)) {
        static const F32 DTR = 0.0174532925f;
        static const F32 screenZ = 10.0f;

        // Sets top of frustum based on FoV-Y and near clipping plane
        F32 top = zPlanes.x * std::tan(DTR * verticalFoV * 0.5f);
        F32 right = aspectRatio * top;
        // Sets right of frustum based on aspect ratio
        F32 frustumshift = (camIOD / 2) * zPlanes.x / screenZ;

        _leftCam.topfrustum = top;
        _leftCam.bottomfrustum = -top;
        _leftCam.leftfrustum = -right + frustumshift;
        _leftCam.rightfrustum = right + frustumshift;
        _leftCam.modeltranslation = camIOD / 2;

        _rightCam.topfrustum = top;
        _rightCam.bottomfrustum = -top;
        _rightCam.leftfrustum = -right - frustumshift;
        _rightCam.rightfrustum = right - frustumshift;
        _rightCam.modeltranslation = -camIOD / 2;

        _anaglyphIOD = camIOD;
    }
    // Set a camera for the requested eye's frustum
    CameraFrustum& tempCam = rightFrustum ? _rightCam : _leftCam;
    // Update the GPU data buffer with the proper projection data based on the
    // eye camera's frustum
    GPUBlock::GPUData& data = _gpuBlock._data;

    data._ProjectionMatrix.frustum(to_float(tempCam.leftfrustum),
                                   to_float(tempCam.rightfrustum),
                                   to_float(tempCam.bottomfrustum),
                                   to_float(tempCam.topfrustum),
                                   zPlanes.x,
                                   zPlanes.y);

    // Translate the matrix to cancel parallax
    data._ProjectionMatrix.translate(tempCam.modeltranslation, 0.0, 0.0);

    data._ZPlanesCombined.xy(zPlanes);

    data._ViewProjectionMatrix.set(data._ViewMatrix * data._ProjectionMatrix);

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
    Kernel& kernel = Application::getInstance().getKernel();
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
  
bool GFXDevice::postProcessingEnabled() const {
    return _enablePostProcessing && 
            //HACK: postprocessing not working with deffered rendering! -Ionut
            getRenderer().getType() != RendererType::RENDERER_DEFERRED_SHADING;
}

/// Depending on the context, either immediately call the function, or pass it
/// to the loading thread via a queue
bool GFXDevice::loadInContext(const CurrentContext& context,
                              const DELEGATE_CBK<>& callback) {
    // Skip invalid callbacks
    if (!callback) {
        return false;
    }
    // If we want and can call the function in the loading thread, add it to the
    // lock-free, single-producer, single-consumer queue
    if (context == CurrentContext::GFX_LOADING_CTX &&
        _state.loadingThreadAvailable()) {
        _state.addToLoadQueue(callback);
    } else {
        callback();
    }
    // The callback is valid and has been processed
    return true;
}

void GFXDevice::threadedLoadCallback() {
    /// Setup per-thread API specifics
    _api->threadedLoadCallback(); 
    // This will be our target container for new items pulled from the queue
    DELEGATE_CBK<> callback;
    // Use an atomic bool to check if the thread is still active
    _state.loadingThreadAvailable(true);
    // Run an infinite loop until we actually request otherwise
    while (!_state.closeLoadingThread()) {
        _state.consumeOneFromQueue();
    }
    // If we close the loading thread, update our atomic bool to make sure the
    // application isn't using it anymore
    _state.loadingThreadAvailable(false);
}
/// Transform our depth buffer to a HierarchicalZ buffer (for occlusion queries)
void GFXDevice::ConstructHIZ() {
    // We don't want to change the viewport or clear the buffer when starting to
    // render to the buffer
    Framebuffer::FramebufferTarget hizTarget;
    // We want to process the data already in the buffer
    hizTarget._clearBuffersOnBind = false;
    // And we calculate the target viewport for each loop
    hizTarget._changeViewport = false;
    hizTarget._drawMask = Framebuffer::FramebufferTarget::BufferMask::DEPTH;
    // The depth buffer's resolution should be equal to the screen's resolution
    vec2<U16> resolution =
        _renderTarget[to_uint(RenderTarget::SCREEN)]
            ->getResolution();
    // We use a special shader that downsamples the buffer
    _HIZConstructProgram->bind();
    // We will use a state block that disables color writes as we will render
    // only a depth image,
    // disables depth testing but allows depth writes
    // Set the depth buffer as the currently active render target
    _renderTarget[to_uint(RenderTarget::DEPTH)]->Begin(hizTarget);
    // Bind the depth texture to the first texture unit
    _renderTarget[to_uint(RenderTarget::DEPTH)]->Bind(
        static_cast<U8>(ShaderProgram::TextureUsage::UNIT0),
        TextureDescriptor::AttachmentType::Depth);
    // Calculate the number of mipmap levels we need to generate
    U32 numLevels = 1 + to_uint(floorf(log2f(fmaxf(to_float(resolution.width),
                                                   to_float(resolution.height)))));
    // Store the current width and height of each mip
    U16 currentWidth = resolution.width;
    U16 currentHeight = resolution.height;
    vec4<I32> previousViewport(_viewport.top());
    // We skip the first level as that's our full resolution image
    for (U16 i = 1; i < numLevels; ++i) {
        // Inform the shader of the resolution we are downsampling from
        _HIZConstructProgram->Uniform("LastMipSize",
                                      vec2<I32>(currentWidth, currentHeight));
        // Calculate next viewport size
        currentWidth /= 2;
        currentHeight /= 2;
        // Ensure that the viewport size is always at least 1x1
        currentWidth = currentWidth > 0 ? currentWidth : 1;
        currentHeight = currentHeight > 0 ? currentHeight : 1;
        // Update the viewport with the new resolution
        updateViewportInternal(vec4<I32>(0, 0, currentWidth, currentHeight));
        // Bind next mip level for rendering but first restrict fetches only to
        // previous level
        _renderTarget[to_uint(RenderTarget::DEPTH)]->SetMipLevel(
            i - 1, i - 1, i, TextureDescriptor::AttachmentType::Depth);
        // Dummy draw command as the full screen quad is generated completely by
        // a geometry shader
        drawPoints(1, _stateDepthOnlyRenderingHash, _HIZConstructProgram);
    }
    updateViewportInternal(previousViewport);
    // Reset mipmap level range for the depth buffer
    _renderTarget[to_uint(RenderTarget::DEPTH)]
        ->ResetMipLevel(TextureDescriptor::AttachmentType::Depth);
    // Unbind the render target
    _renderTarget[to_uint(RenderTarget::DEPTH)]->End();
}

/// Find an unused primitive object or create a new one and return it
IMPrimitive* GFXDevice::getOrCreatePrimitive(bool allowPrimitiveRecycle) {
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
        tempPriv = newIMP();
        // And add it to our container. The GFXDevice class is responsible for
        // deleting these!
        _imInterfaces.push_back(tempPriv);
    }
    // Toggle zombification of the primitive on or off depending on our request
    tempPriv->_canZombify = allowPrimitiveRecycle;

    return tempPriv;
}
/// Renders the result of plotting the specified 2D graph
void GFXDevice::plot2DGraph(const Util::GraphPlot2D& plot2D,
                            const vec4<U8>& color) {
    if (!plot2D.empty()) {
        Util::GraphPlot3D plot3D(plot2D._plotName);
        plot3D._coords.reserve(plot2D._coords.size());
        for (const vec2<F32>& coords : plot2D._coords) {
            vectorAlg::emplace_back(plot3D._coords, coords, 0.0f);
        }
        plot3DGraph(plot3D, color);
    }
}

/// Renders the result of plotting the specified 3D graph
void GFXDevice::plot3DGraph(const Util::GraphPlot3D& plot3D,
                            const vec4<U8>& color) {
    if (!plot3D.empty()) {
        vectorImpl<Line> plotLines;
        const vectorImpl<vec3<F32>>& coords = plot3D._coords;

        vectorAlg::vecSize coordCount = coords.size();
        plotLines.reserve(coordCount / 2);
        vectorImpl<vec3<F32>>::const_iterator it;
        for (it = std::begin(coords); it != std::end(coords); ++it) {
            vectorAlg::emplace_back(plotLines, *(it), *(it++), color);
        }

        IMPrimitive* primitive = GFX_DEVICE.getOrCreatePrimitive();
        primitive->name("3DGraphPrimitive");
        drawLines(*primitive, plotLines, mat4<F32>(), vec4<I32>(), false);
    }
}

/// Extract the pixel data from the main render target's first color attachment
/// and save it as a TGA image
void GFXDevice::Screenshot(const stringImpl& filename) {
    // Get the screen's resolution
    const vec2<U16>& resolution =
        _renderTarget[to_uint(RenderTarget::SCREEN)]
            ->getResolution();
    // Allocate sufficiently large buffers to hold the pixel data
    U32 bufferSize = resolution.width * resolution.height * 4;
    U8* imageData = MemoryManager_NEW U8[bufferSize];
    // Read the pixels from the main render target (RGBA16F)
    _renderTarget[to_uint(RenderTarget::SCREEN)]->ReadData(
        GFXImageFormat::RGBA, GFXDataFormat::UNSIGNED_BYTE, imageData);
    // Save to file
    ImageTools::SaveSeries(filename,
                           vec2<U16>(resolution.width, resolution.height), 32,
                           imageData);
    // Delete local buffers
    MemoryManager::DELETE_ARRAY(imageData);
}
};
