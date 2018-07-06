#include "Headers/GFXDevice.h"
#include "Headers/RenderStateBlock.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Core/Math/Headers/Plane.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Utility/Headers/ImageTools.h"
#include "Managers/Headers/SceneManager.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#include "Platform/Video/Shaders/Headers/ShaderManager.h"

#ifdef FORCE_HIGHPERFORMANCE_GPU
extern "C" {
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

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
    : _api(nullptr), _renderStage(RenderStage::INVALID_STAGE) {
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
    // Integers
    _stateExclusionMask = 0;
    FRAME_COUNT = 0;
    FRAME_DRAW_CALLS = 0;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    // Floats
    _currentLineWidth = 1.0;
    _previousLineWidth = 1.0;
    _interpolationFactor = 1.0;
    // Booleans
    _2DRendering = false;
    _drawDebugAxis = false;
    _enableAnaglyph = false;
    _viewportUpdate = false;
    _rasterizationEnabled = true;
    _enablePostProcessing = false;
    // Enumerated Types
    _shadowDetailLevel = RenderDetailLevel::DETAIL_HIGH;
    _GPUVendor = GPUVendor::COUNT;
    _API_ID = RenderAPI::COUNT;
    // Utility cameras
    _2DCamera = MemoryManager_NEW FreeFlyCamera();
    _2DCamera->lockView(true);
    _2DCamera->lockFrustum(true);
    _cubeCamera = MemoryManager_NEW FreeFlyCamera();
    // Clipping planes
    _clippingPlanes.resize(Config::MAX_CLIP_PLANES, Plane<F32>(0, 0, 0, 0));
    // Render targets
    for (Framebuffer*& renderTarget : _renderTarget) {
        renderTarget = nullptr;
    }
    // Add our needed app-wide render passes. RenderPassManager is responsible
    // for deleting these!
    RenderPassManager::getOrCreateInstance().addRenderPass("diffusePass", 1);
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

GFXDevice::~GFXDevice() {}

/// A draw command is composed of a target buffer and a command. The command
/// part is processed here
bool GFXDevice::setBufferData(const GenericDrawCommand& cmd) {
    // We also need a valid draw command ID so we can index the node buffer
    // properly
    DIVIDE_ASSERT(
        cmd.cmd().baseInstance < std::max(static_cast<U32>(_matricesData.size()), 1u) &&
            cmd.cmd().baseInstance >= 0,
        "GFXDevice error: Invalid draw ID encountered!");
    if (cmd.instanceCount() == 0 || cmd.drawCount() == 0) {
        return false;
    }
    // We need a valid shader as no fixed function pipeline is available
    DIVIDE_ASSERT(cmd.shaderProgram() != nullptr,
                  "GFXDevice error: Draw shader state is not valid for the "
                  "current draw operation!");
    // Try to bind the shader program. If it failed to load, or isn't loaded
    // yet, cancel the draw request for this frame
    if (!cmd.shaderProgram()->bind()) {
        return false;
    }
    // Finally, set the proper render states
    setStateBlock(cmd.stateHash());
    // Continue with the draw call
    return true;
}

/// This is just a short-circuit system (hack) to send a list of points to the
/// shader
/// It's used, mostly, to draw full screen quads using geometry shaders
void GFXDevice::drawPoints(U32 numPoints, size_t stateHash,
                           ShaderProgram* const shaderProgram) {
    // We need a valid amount of points. Check lower limit
    if (numPoints == 0) {
        Console::errorfn(Locale::get("ERROR_GFX_POINTS_UNDERFLOW"));
        return;
    }
    // Also check upper limit
    if (numPoints > Config::MAX_POINTS_PER_BATCH) {
        Console::errorfn(Locale::get("ERROR_GFX_POINTS_OVERFLOW"));
        return;
    }
    // We require a state hash value to set proper states
    _defaultDrawCmd.stateHash(stateHash);
    // We also require a shader program (although it may already be bound.
    // Better safe ...)
    _defaultDrawCmd.shaderProgram(shaderProgram);
    // If the draw command was successfully parsed
    if (setBufferData(_defaultDrawCmd)) {
        // Tell the rendering API to upload the needed number of points
        drawPoints(numPoints);
    }
}

/// GUI specific elements, require custom handling by the rendering API
void GFXDevice::drawGUIElement(GUIElement* guiElement) {
    DIVIDE_ASSERT(guiElement != nullptr,
                  "GFXDevice error: Invalid GUI element specified for the "
                  "drawGUI command!");
    // Skip hidden elements
    if (!guiElement->isVisible()) {
        return;
    }
    // Set the elements render states
    setStateBlock(guiElement->getStateBlockHash());
    // Choose the appropriate rendering path
    switch (guiElement->getType()) {
        case GUIType::GUI_TEXT: {
            const GUIText& text = static_cast<GUIText&>(*guiElement);
            drawText(text, text.getPosition());
        } break;
        case GUIType::GUI_FLASH: {
            static_cast<GUIFlash*>(guiElement)->playMovie();
        } break;
        default: {
            // not supported
        } break;
    };
    // Update internal timer
    guiElement->lastDrawTimer(Time::ElapsedMicroseconds());
}

/// Submit a single draw command
void GFXDevice::submitRenderCommand(const GenericDrawCommand& cmd) {
    DIVIDE_ASSERT(cmd.sourceBuffer() != nullptr,
                  "GFXDevice error: Invalid vertex buffer submitted!");
    // We may choose the instance count programmatically, and it may turn out to
    // be 0, so skip draw
    if (setBufferData(cmd)) {
        // Same rules about pre-processing the draw command apply
        cmd.sourceBuffer()->Draw(cmd);
    }
}

/// Submit multiple draw commands that use the same source buffer (e.g. terrain
/// or batched meshes)
void GFXDevice::submitRenderCommand(
    const vectorImpl<GenericDrawCommand>& cmds) {
    // Ideally, we would merge all of the draw commands in a command buffer,
    // sort by state/shader/etc and submit a single render call
    STUBBED("Batch by state hash and submit multiple draw calls! - Ionut");
    // That feature will be added later, so, for now, submit each command
    // manually
    for (const GenericDrawCommand& cmd : cmds) {
        // Data validation is handled in the single command version
        submitRenderCommand(cmd);
    }
}

void GFXDevice::processNodeRenderData(
    RenderingComponent::NodeRenderData& data) {
    for (RenderingComponent::ShaderBufferList::value_type& it :
         data._shaderBuffers) {
        if (it.second) {
            it.second->Bind(it.first);
        }
    }

    makeTexturesResident(data._textureData);
    submitRenderCommand(data._drawCommands);
}

/// Generate a cube texture and store it in the provided framebuffer
void GFXDevice::generateCubeMap(Framebuffer& cubeMap, const vec3<F32>& pos,
                                const DELEGATE_CBK<>& renderFunction,
                                const vec2<F32>& zPlanes,
                                const RenderStage& renderStage) {
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
        getRenderer().render(renderFunction, GET_ACTIVE_SCENE()->renderState());
    }
    // Resolve our render target
    cubeMap.End();
    // Return to our previous rendering stage
    setRenderStage(prevRenderStage);
    // Restore our previous camera
    kernel.getCameraMgr().popActiveCamera();
}

/// Try to find the render state block that matches our specified descriptor.
/// Create a new one if none are found
size_t GFXDevice::getOrCreateStateBlock(
    const RenderStateBlockDescriptor& descriptor) {
    // Each combination of render states has a unique hash value
    size_t hashValue = descriptor.getHash();
    // Find the corresponding render state block
    if (_stateBlockMap.find(hashValue) == std::end(_stateBlockMap)) {
        // Create a new one if none are found. The GFXDevice class is
        // responsible for deleting these!
        hashAlg::emplace(_stateBlockMap, hashValue,
                         MemoryManager_NEW RenderStateBlock(descriptor));
    }
    // Return the descriptor's hash value
    return hashValue;
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

/// Return the descriptor of the render state block defined by the specified
/// hash value.
/// The state block must be created prior to calling this!
const RenderStateBlockDescriptor& GFXDevice::getStateBlockDescriptor(
    size_t renderStateBlockHash) const {
    // Find the render state block associated with the received hash value
    RenderStateMap::const_iterator it =
        _stateBlockMap.find(renderStateBlockHash);
    // Assert if it doesn't exist. Avoids programming errors.
    DIVIDE_ASSERT(it != std::end(_stateBlockMap),
                  "GFXDevice error: Invalid render state block hash specified "
                  "for getStateBlockDescriptor!");
    // Return the state block's descriptor
    return it->second->getDescriptor();
}

/// The main entry point for any resolution change request
void GFXDevice::changeResolution(U16 w, U16 h) {
    // Make sure we are in a valid state that allows resolution updates
    if (_renderTarget[to_uint(RenderTarget::RENDER_TARGET_SCREEN)] !=
        nullptr) {
        // Update resolution only if it's different from the current one.
        // Avoid resolution change on minimize so we don't thrash render targets
        if (vec2<U16>(w, h) ==
                _renderTarget[to_uint(RenderTarget::RENDER_TARGET_SCREEN)]
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
    // Inform rendering API of the resolution change
    _api->changeResolution(w, h);
    // Set the viewport to be the entire window. Force the update so we don't
    // push a new value (we replace the old)
    forceViewportInternal(vec4<I32>(0, 0, w, h));
    // Make sure the main viewport is the only one active. Never change
    // resolution while rendering to a render target
    DIVIDE_ASSERT(_viewport.size() == 1,
                  "GFXDevice error: changeResolution called while not "
                  "rendering to screen!");
    // Inform the Kernel
    Kernel::updateResolutionCallback(w, h);
    // Update post-processing render targets and buffers
    PostFX::getInstance().updateResolution(w, h);
    // Refresh shader programs
    ShaderManager::getInstance().refreshShaderData();
    // Update the 2D camera so it matches our new rendering viewport
    _2DCamera->setProjection(vec4<F32>(0, w, 0, h), vec2<F32>(-1, 1));
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
    if (mode == MATRIX_MODE::VIEW_PROJECTION_MATRIX) {
        mat.set(_gpuBlock._ViewProjectionMatrix);
    } else if (mode == MATRIX_MODE::VIEW_MATRIX) {
        mat.set(_gpuBlock._ViewMatrix);
    } else if (mode == MATRIX_MODE::PROJECTION_MATRIX) {
        mat.set(_gpuBlock._ProjectionMatrix);
    } else if (mode == MATRIX_MODE::TEXTURE_MATRIX) {
        mat.identity();
        Console::errorfn(Locale::get("ERROR_TEXTURE_MATRIX_ACCESS"));
    } else if (mode == MATRIX_MODE::VIEW_INV_MATRIX) {
        _gpuBlock._ViewMatrix.getInverse(mat);
    } else if (mode == MATRIX_MODE::PROJECTION_INV_MATRIX) {
        _gpuBlock._ProjectionMatrix.getInverse(mat);
    } else if (mode == MATRIX_MODE::VIEW_PROJECTION_INV_MATRIX) {
        _gpuBlock._ViewProjectionMatrix.getInverse(mat);
    } else {
        DIVIDE_ASSERT(
            false,
            "GFXDevice error: attempted to query an invalid matrix target!");
    }
}

/// Update the internal GPU data buffer with the clip plane values
void GFXDevice::updateClipPlanes() {
    for (U8 i = 0; i < Config::MAX_CLIP_PLANES; ++i) {
        _gpuBlock._clipPlanes[i] = _clippingPlanes[i].getEquation();
    }
    // We flush the entire buffer on update to inform the GPU that we don't need
    // the previous data.
    // Might avoid some driver sync
    _gfxDataBuffer->SetData(&_gpuBlock);
}

/// Update the internal GPU data buffer with the updated viewport dimensions
void GFXDevice::updateViewportInternal(const vec4<I32>& viewport) {
    // Change the viewport on the Rendering API level
    changeViewport(viewport);
    // Update the buffer with the new value
    _gpuBlock._ViewPort.set(viewport.x, viewport.y, viewport.z, viewport.w);
    // Update the buffer on the GPU
    _gfxDataBuffer->SetData(&_gpuBlock);
}

/// Update the virtual camera's matrices and upload them to the GPU
F32* GFXDevice::lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& eyePos) {
    bool updated = false;
    if (eyePos != _gpuBlock._cameraPosition) {
        _gpuBlock._cameraPosition.set(eyePos);
        updated = true;
    }
    if (viewMatrix != _gpuBlock._ViewMatrix) {
        _gpuBlock._ViewMatrix.set(viewMatrix);
        updated = true;
    }
    if (updated) {
        _gpuBlock._ViewProjectionMatrix.set(_gpuBlock._ViewMatrix *
                                            _gpuBlock._ProjectionMatrix);
        _gfxDataBuffer->SetData(&_gpuBlock);
    }
    return _gpuBlock._ViewMatrix.mat;
}

/// Enable an orthographic projection and upload the corresponding matrices to
/// the GPU
F32* GFXDevice::setProjection(const vec4<F32>& rect, const vec2<F32>& planes) {
    _gpuBlock._ProjectionMatrix.ortho(rect.x, rect.y, rect.z, rect.w, planes.x,
                                      planes.y);
    _gpuBlock._ZPlanesCombined.x = planes.x;
    _gpuBlock._ZPlanesCombined.y = planes.y;
    _gpuBlock._ViewProjectionMatrix.set(_gpuBlock._ViewMatrix *
                                        _gpuBlock._ProjectionMatrix);
    _gfxDataBuffer->SetData(&_gpuBlock);

    return _gpuBlock._ProjectionMatrix.mat;
}

/// Enable a perspective projection and upload the corresponding matrices to the
/// GPU
F32* GFXDevice::setProjection(F32 FoV, F32 aspectRatio,
                              const vec2<F32>& planes) {
    _gpuBlock._ProjectionMatrix.perspective(Angle::DegreesToRadians(FoV),
                                            aspectRatio, planes.x, planes.y);
    _gpuBlock._ZPlanesCombined.x = planes.x;
    _gpuBlock._ZPlanesCombined.y = planes.y;
    _gpuBlock._ViewProjectionMatrix.set(_gpuBlock._ViewMatrix *
                                        _gpuBlock._ProjectionMatrix);
    _gfxDataBuffer->SetData(&_gpuBlock);

    return _gpuBlock._ProjectionMatrix.mat;
}

/// Calculate a frustum for the requested eye (left-right frustum) for anaglyph
/// rendering
void GFXDevice::setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes,
                                   F32 aspectRatio, F32 verticalFoV,
                                   bool rightFrustum) {
    // Only update frustum values if the interocular distance changed from the
    // previous request
    if (!FLOAT_COMPARE(_anaglyphIOD, camIOD)) {
        static const D32 DTR = 0.0174532925;
        static const D32 screenZ = 10.0;

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
    _gpuBlock._ProjectionMatrix.frustum(
        tempCam.leftfrustum, tempCam.rightfrustum, tempCam.bottomfrustum,
        tempCam.topfrustum, zPlanes.x, zPlanes.y);
    // Translate the matrix to cancel parallax
    _gpuBlock._ProjectionMatrix.translate(tempCam.modeltranslation, 0.0, 0.0);

    _gpuBlock._ZPlanesCombined.x = zPlanes.x;
    _gpuBlock._ZPlanesCombined.y = zPlanes.y;
    _gpuBlock._ViewProjectionMatrix.set(_gpuBlock._ViewMatrix *
                                        _gpuBlock._ProjectionMatrix);
    // Upload the new data to the GPU
    _gfxDataBuffer->SetData(&_gpuBlock);
}

/// Enable or disable 2D rendering mode (orthographic projection, no depth
/// reads)
void GFXDevice::toggle2D(bool state) {
    // Remember the previous state hash
    static size_t previousStateBlockHash = 0;
    // Prevent double 2D toggle to the same state (e.g. in a loop)
    DIVIDE_ASSERT(
        (state && !_2DRendering) || (!state && _2DRendering),
        "GFXDevice error: double toggle2D call with same value detected!");
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

/// Toggle post-processing on or off
void GFXDevice::postProcessingEnabled(const bool state) {
    // Avoid double toggle to same state
    if (_enablePostProcessing != state) {
        _enablePostProcessing = state;
        // If we enable post-processing, we must call idle() once to make sure
        // it pulled updated values
        if (state) {
            // If idle isn't called before rendering, outdated data may cause a
            // crash or rendering artifacts
            PostFX::getInstance().idle();
        }
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
void GFXDevice::forceViewportInternal(const vec4<I32>& viewport) {
    // Clear the viewport stack
    while (!_viewport.empty()) {
        _viewport.pop();
    }
    // Set the new viewport
    _viewport.push(viewport);
    updateViewportInternal(viewport);
    // The forced viewport can't be popped
    _viewportUpdate = false;
}

/// Prepare the list of visible nodes for rendering
void GFXDevice::processVisibleNodes(
    const vectorImpl<SceneGraphNode*>& visibleNodes) {
    // If there aren't any nodes visible in the current pass, don't update
    // anything
    if (visibleNodes.empty()) {
        return;
    }
    vectorAlg::vecSize nodeCount = visibleNodes.size();
    // Pass the list of nodes to the renderer for a pre-render pass
    getRenderer().processVisibleNodes(visibleNodes, _gpuBlock);
    // Generate and upload all lighting data
    LightManager::getInstance().updateAndUploadLightData(_gpuBlock._ViewMatrix);
    // The first entry has identity values (e.g. for rendering points)
    _matricesData.reserve(nodeCount + 1);
    _matricesData.resize(1);
    // Loop over the list of nodes
    for (SceneGraphNode* const crtNode : visibleNodes) {
        NodeData temp;
        // Extract transform data
        const PhysicsComponent* const transform =
            crtNode->getComponent<PhysicsComponent>();
        // If we have valid transform data ...
        if (transform) {
            // ... get the node's world matrix properly interpolated
            temp._matrix[0].set(
                crtNode->getComponent<PhysicsComponent>()->getWorldMatrix(
                    _interpolationFactor));
            // Calculate the normal matrix (world * view)
            // If the world matrix is uniform scaled, inverseTranspose is a
            // double transpose (no-op) so we can skip it
            temp._matrix[1].set(
                mat3<F32>(temp._matrix[0] * _gpuBlock._ViewMatrix));
            if (!transform->isUniformScaled()) {
                // Non-uniform scaling requires an inverseTranspose to negate
                // scaling contribution but preserve rotation
                temp._matrix[1].set(mat3<F32>(temp._matrix[0]));
                temp._matrix[1].inverseTranspose();
                temp._matrix[1] *= _gpuBlock._ViewMatrix;
            }
        } else {
            // Nodes without transforms are considered as using identity
            // matrices
            temp._matrix[0].identity();
            temp._matrix[1].identity();
        }
        // Since the normal matrix is 3x3, we can use the extra row and column
        // to store additional data
        temp._matrix[1].element(3, 2, true) =
            static_cast<F32>(LightManager::getInstance().getLights().size());
        temp._matrix[1].element(3, 3, true) = static_cast<F32>(
            crtNode->getComponent<AnimationComponent>()
                ? crtNode->getComponent<AnimationComponent>()->boneCount()
                : 0);

        RenderingComponent* const renderable =
            crtNode->getComponent<RenderingComponent>();
        // Get the color matrix (diffuse, ambient, specular, etc.)
        renderable->getMaterialColorMatrix(temp._matrix[2]);
        // Get the material property matrix (alpha test, texture count,
        // texture operation, etc.)
        renderable->getMaterialPropertyMatrix(temp._matrix[3]);

        _matricesData.push_back(temp);
    }
    // Once the CPU-side buffer is filled, upload the buffer to the GPU
    _nodeBuffer->UpdateData(0, (nodeCount + 1) * sizeof(NodeData),
                            _matricesData.data());
}

void GFXDevice::buildDrawCommands(
    const vectorImpl<SceneGraphNode*>& visibleNodes,
    SceneRenderState& sceneRenderState) {
    // If there aren't any nodes visible in the current pass, don't update
    // anything
    if (visibleNodes.empty()) {
        return;
    }
        
    // Reset previously generated commands
    _nonBatchedCommands.reserve(visibleNodes.size() + 1);
    _nonBatchedCommands.resize(0);
    _nonBatchedCommands.push_back(_defaultDrawCmd);
        
    U32 drawID = 0;
    // Loop over the list of nodes to generate a new command list
    RenderStage currentStage = getRenderStage();
    for (SceneGraphNode* node : visibleNodes) {
        vectorImpl<GenericDrawCommand>& nodeDrawCommands =
            node->getComponent<RenderingComponent>()->getDrawCommands(
                sceneRenderState, currentStage);
        drawID += 1;
        for (GenericDrawCommand& cmd : nodeDrawCommands) {
            cmd.drawID(drawID);
            _nonBatchedCommands.push_back(cmd);
        }
    }

    // Extract the specific rendering commands from the draw commands
    // Rendering commands are stored in GPU memory. Draw commands are not.
    _drawCommandsCache.reserve(_nonBatchedCommands.size());
    _drawCommandsCache.resize(0);
    for (const GenericDrawCommand& cmd : _nonBatchedCommands) {
        _drawCommandsCache.push_back(cmd.cmd());
    }

    // Upload the rendering commands to the GPU memory
    uploadDrawCommands(_drawCommandsCache);
    
}

bool GFXDevice::batchCommands(GenericDrawCommand& previousIDC,
                              GenericDrawCommand& currentIDC) const {
    if (!previousIDC.sourceBuffer() || !currentIDC.sourceBuffer()) {
        return false;
    }
    if (!previousIDC.shaderProgram() || !currentIDC.shaderProgram()) {
        return false;
    }
    // Batchable commands must share the same buffer
    if (previousIDC.sourceBuffer()->getGUID() ==
            currentIDC.sourceBuffer()->getGUID() &&
        // And the same shader program
        previousIDC.shaderProgram()->getID() ==
            currentIDC.shaderProgram()->getID() &&
        // Different states aren't batchable currently
        previousIDC.stateHash() == currentIDC.stateHash() &&
        // We need the same primitive type
        previousIDC.primitiveType() == currentIDC.primitiveType() &&
        previousIDC.renderWireframe() == currentIDC.renderWireframe()) 
    {
        // If the rendering commands are batchable, increase the draw count for
        // the previous one
        previousIDC.drawCount(previousIDC.drawCount() + 1);
        // And set the current command's draw count to zero so it gets removed
        // from the list later on
        currentIDC.drawCount(0);
        return true;
    }

    return false;
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
    if (context == CurrentContext::GFX_LOADING_CONTEXT &&
        _state.loadingThreadAvailable()) {
        while (!_state.getLoadQueue().push(callback))
            ;
    } else {
        callback();
    }
    // The callback is valid and has been processed
    return true;
}

/// Transform our depth buffer to a HierarchicalZ buffer (for occlusion queries)
void GFXDevice::ConstructHIZ() {
    // We don't want to change the viewport or clear the buffer when starting to
    // render to the buffer
    static Framebuffer::FramebufferTarget hizTarget;
    // We want to process the data already in the buffer
    hizTarget._clearBuffersOnBind = false;
    // And we calculate the target viewport for each loop
    hizTarget._changeViewport = false;
    // The depth buffer's resolution should be equal to the screen's resolution
    vec2<U16> resolution =
        _renderTarget[to_uint(RenderTarget::RENDER_TARGET_DEPTH)]
            ->getResolution();
    // We use a special shader that downsamples the buffer
    _HIZConstructProgram->bind();
    // We will use a state block that disables color writes as we will render
    // only a depth image,
    // disables depth testing but allows depth writes
    // Set the depth buffer as the currently active render target
    _renderTarget[to_uint(RenderTarget::RENDER_TARGET_DEPTH)]->Begin(
        hizTarget);
    // Bind the depth texture to the first texture unit
    _renderTarget[to_uint(RenderTarget::RENDER_TARGET_DEPTH)]->Bind(
        to_uint(ShaderProgram::TextureUsage::TEXTURE_UNIT0),
        TextureDescriptor::AttachmentType::Depth);
    // Calculate the number of mipmap levels we need to generate
    U16 numLevels = 1 + (U16)floorf(log2f(fmaxf((F32)resolution.width,
                                                (F32)resolution.height)));
    // Store the current width and height of each mip
    U16 currentWidth = resolution.width;
    U16 currentHeight = resolution.height;
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
        GFX::ScopedViewport viewport(
            vec4<I32>(0, 0, currentWidth, currentHeight));
        // Bind next mip level for rendering but first restrict fetches only to
        // previous level
        _renderTarget[to_uint(RenderTarget::RENDER_TARGET_DEPTH)]
            ->SetMipLevel(i - 1, i - 1, i,
                          TextureDescriptor::AttachmentType::Depth);
        // Dummy draw command as the full screen quad is generated completely by
        // a geometry shader
        drawPoints(1, _stateDepthOnlyRenderingHash, _HIZConstructProgram);
    }
    // Reset mipmap level range for the depth buffer
    _renderTarget[to_uint(RenderTarget::RENDER_TARGET_DEPTH)]
        ->ResetMipLevel(TextureDescriptor::AttachmentType::Depth);
    // Unbind the render target
    _renderTarget[to_uint(RenderTarget::RENDER_TARGET_DEPTH)]->End();
}

/// Find an unused primitive object or create a new one and return it
IMPrimitive* GFXDevice::getOrCreatePrimitive(bool allowPrimitiveRecycle) {
    IMPrimitive* tempPriv = nullptr;
    // Find an available and unused primitive (a zombie primitive)
    vectorImpl<IMPrimitive*>::iterator it = std::find_if(
        std::begin(_imInterfaces), std::end(_imInterfaces),
        [](IMPrimitive* const priv) { return (priv && !priv->inUse()); });
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
                            const vec4<U8>& color) const {
    if (plot2D.empty()) {
        return;
    }
    Util::GraphPlot3D plot3D;
    plot3D._plotName = plot2D._plotName;
    plot3D._coords.reserve(plot2D._coords.size());
    for (const vec2<F32>& coords : plot2D._coords) {
        vectorAlg::emplace_back(plot3D._coords, coords.x, coords.y, 0.0f);
    }
    plot3DGraph(plot3D, color);
}

/// Renders the result of plotting the specified 3D graph
void GFXDevice::plot3DGraph(const Util::GraphPlot3D& plot3D,
                            const vec4<U8>& color) const {
    if (plot3D.empty()) {
        return;
    }
    vectorImpl<Line> plotLines;
    plotLines.reserve(plot3D._coords.size());
    vectorAlg::vecSize coordCount = plot3D._coords.size();
    for (vectorAlg::vecSize i = 0; i < coordCount - 1; ++i) {
        const vec3<F32>& startCoord = plot3D._coords[i];
        const vec3<F32>& endCoord = plot3D._coords[i + 1];
        vectorAlg::emplace_back(plotLines, startCoord, endCoord, color);
    }
}

/// Draw the outlines of a box defined by min and max as extents using the
/// specified world matrix
void GFXDevice::drawBox3D(const vec3<F32>& min, const vec3<F32>& max,
                          const vec4<U8>& color) {
    // Grab an available primitive
    IMPrimitive* priv = getOrCreatePrimitive();
    // Prepare it for rendering lines
    priv->_hasLines = true;
    priv->_lineWidth = 4.0f;
    priv->stateHash(_defaultStateBlockHash);
    // Create the object
    priv->beginBatch();
    // Set it's color
    priv->attribute4ub("inColorData", color);
    // Draw the bottom loop
    priv->begin(PrimitiveType::LINE_LOOP);
    priv->vertex(min.x, min.y, min.z);
    priv->vertex(max.x, min.y, min.z);
    priv->vertex(max.x, min.y, max.z);
    priv->vertex(min.x, min.y, max.z);
    priv->end();
    // Draw the top loop
    priv->begin(PrimitiveType::LINE_LOOP);
    priv->vertex(min.x, max.y, min.z);
    priv->vertex(max.x, max.y, min.z);
    priv->vertex(max.x, max.y, max.z);
    priv->vertex(min.x, max.y, max.z);
    priv->end();
    // Connect the top to the bottom
    priv->begin(PrimitiveType::LINES);
    priv->vertex(min.x, min.y, min.z);
    priv->vertex(min.x, max.y, min.z);
    priv->vertex(max.x, min.y, min.z);
    priv->vertex(max.x, max.y, min.z);
    priv->vertex(max.x, min.y, max.z);
    priv->vertex(max.x, max.y, max.z);
    priv->vertex(min.x, min.y, max.z);
    priv->vertex(min.x, max.y, max.z);
    priv->end();
    // Finish our object
    priv->endBatch();
}

/// Render a list of lines within the specified constraints
void GFXDevice::drawLines(const vectorImpl<Line>& lines,
                          const mat4<F32>& globalOffset,
                          const vec4<I32>& viewport, const bool inViewport,
                          const bool disableDepth) {
    // Check if we have a valid list. The list can be programmatically
    // generated, so this check is required
    if (lines.empty()) {
        return;
    }
    // Grab an available primitive
    IMPrimitive* priv = getOrCreatePrimitive();
    // Prepare it for line rendering
    priv->_hasLines = true;
    priv->_lineWidth = 5.0f;
    priv->stateHash(getDefaultStateBlock(disableDepth));
    // Set the world matrix
    priv->worldMatrix(globalOffset);
    // If we need to render it into a specific viewport, set the pre and post
    // draw functions to set up the
    // needed viewport rendering (e.g. axis lines)
    if (inViewport) {
        priv->setRenderStates(
            DELEGATE_BIND(&GFXDevice::setViewport, this, viewport),
            DELEGATE_BIND(&GFXDevice::restoreViewport, this));
    }
    // Create the object containing all of the lines
    priv->beginBatch();
    priv->attribute4ub("inColorData", lines[0]._color);
    // Set the mode to line rendering
    priv->begin(PrimitiveType::LINES);
    // Add every line in the list to the batch
    for (const Line& line : lines) {
        priv->attribute4ub("inColorData", line._color);
        priv->vertex(line._startPoint);
        priv->vertex(line._endPoint);
    }
    priv->end();
    // Finish our object
    priv->endBatch();
}

/// Extract the pixel data from the main render target's first color attachment
/// and save it as a TGA image
void GFXDevice::Screenshot(char* filename) {
    // Get the screen's resolution
    const vec2<U16>& resolution =
        _renderTarget[to_uint(RenderTarget::RENDER_TARGET_SCREEN)]
            ->getResolution();
    // Allocate sufficiently large buffers to hold the pixel data
    U32 bufferSize = resolution.width * resolution.height * 4;
    U8* imageData = MemoryManager_NEW U8[bufferSize];
    // Read the pixels from the main render target (RGBA16F)
    _renderTarget[to_uint(RenderTarget::RENDER_TARGET_SCREEN)]->ReadData(
        GFXImageFormat::RGBA, GFXDataFormat::UNSIGNED_BYTE, imageData);
    // Save to file
    ImageTools::SaveSeries(filename,
                           vec2<U16>(resolution.width, resolution.height), 32,
                           imageData);
    // Delete local buffers
    MemoryManager::DELETE_ARRAY(imageData);
}
};
