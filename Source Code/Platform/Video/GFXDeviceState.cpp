#include "config.h"

#include "Headers/GFXDevice.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Direct3D/Headers/DXWrapper.h"

namespace Divide {

/// Create a display context using the selected API and create all of the needed
/// primitives needed for frame rendering
ErrorCode GFXDevice::initRenderingAPI(I32 argc, char** argv, const vec2<U16>& renderResolution) {

    ErrorCode hardwareState = createAPIInstance();
    if (hardwareState == ErrorCode::NO_ERR) {
        // Initialize the rendering API
        hardwareState = _api->initRenderingAPI(argc, argv);
    }

    if (hardwareState != ErrorCode::NO_ERR) {
        // Validate initialization
        return hardwareState;
    }

    stringImpl refreshRates;
    vectorAlg::vecSize displayCount = gpuState().getDisplayCount();
    for (vectorAlg::vecSize idx = 0; idx < displayCount; ++idx) {
        const vectorImpl<GPUState::GPUVideoMode>& registeredModes = gpuState().getDisplayModes(idx);
        Console::printfn(Locale::get(_ID("AVAILABLE_VIDEO_MODES")), idx, registeredModes.size());

        for (const GPUState::GPUVideoMode& mode : registeredModes) {
            // Optionally, output to console/file each display mode
            refreshRates = Util::StringFormat("%d", mode._refreshRate.front());
            vectorAlg::vecSize refreshRateCount = mode._refreshRate.size();
            for (vectorAlg::vecSize i = 1; i < refreshRateCount; ++i) {
                refreshRates += Util::StringFormat(", %d", mode._refreshRate[i]);
            }
            Console::printfn(Locale::get(_ID("CURRENT_DISPLAY_MODE")),
                mode._resolution.width,
                mode._resolution.height,
                mode._bitDepth,
                mode._formatName.c_str(),
                refreshRates.c_str());
        }
    }

    // Initialize the shader manager
    ShaderProgram::initStaticData();
    // Create an immediate mode shader used for general purpose rendering (e.g.
    // to mimic the fixed function pipeline)
    _imShader = ShaderProgram::defaultShader();
    _imShaderTextureFlag = _imShader->getUniformLocation("useTexture");
    _imShaderWorldMatrix = _imShader->getUniformLocation("dvd_WorldMatrix");

    DIVIDE_ASSERT(_imShader != nullptr,
                  "GFXDevice error: No immediate mode emulation shader available!");
    PostFX::createInstance();
    // Create a shader buffer to store the following info:
    // ViewMatrix, ProjectionMatrix, ViewProjectionMatrix, CameraPositionVec, ViewportRec, zPlanesVec4 and ClipPlanes[MAX_CLIP_PLANES]
    // It should translate to (as seen by OpenGL) a uniform buffer without persistent mapping.
    // (Many small updates with BufferSubData are recommended with the target usage of the buffer)
    _gfxDataBuffer = newSB(1, false, false, BufferUpdateFrequency::OFTEN);
    _gfxDataBuffer->create(1, sizeof(GPUBlock));
    _gfxDataBuffer->bind(ShaderBufferLocation::GPU_BLOCK);

    // Utility cameras
    CameraManager& cameraMgr = Application::instance().kernel().getCameraMgr();
    _2DCamera = cameraMgr.createCamera("2DRenderCamera", Camera::CameraType::FREE_FLY);
    _2DCamera->lockView(true);
    _cubeCamera = cameraMgr.createCamera("_gfxCubeCamera", Camera::CameraType::FREE_FLY);
    _dualParaboloidCamera = cameraMgr.createCamera("_gfxParaboloidCamera", Camera::CameraType::FREE_FLY);

    // Create general purpose render state blocks
    RenderStateBlock defaultState;
    _defaultStateBlockHash = defaultState.getHash();

    RenderStateBlock defaultStateNoDepth;
    defaultStateNoDepth.setZRead(false);
    _defaultStateNoDepthHash = defaultStateNoDepth.getHash();

    RenderStateBlock state2DRendering;
    state2DRendering.setCullMode(CullMode::NONE);
    state2DRendering.setZRead(false);
    _state2DRenderingHash = state2DRendering.getHash();

    RenderStateBlock stateDepthOnlyRendering;
    stateDepthOnlyRendering.setColourWrites(false, false, false, false);
    stateDepthOnlyRendering.setZFunc(ComparisonFunction::ALWAYS);
    _stateDepthOnlyRenderingHash = stateDepthOnlyRendering.getHash();

    // The general purpose render state blocks are both mandatory and must
    // differ from each other at a state hash level
    DIVIDE_ASSERT(_stateDepthOnlyRenderingHash != _state2DRenderingHash,
                  "GFXDevice error: Invalid default state hash detected!");
    DIVIDE_ASSERT(_state2DRenderingHash != _defaultStateNoDepthHash,
                  "GFXDevice error: Invalid default state hash detected!");
    DIVIDE_ASSERT(_defaultStateNoDepthHash != _defaultStateBlockHash,
                  "GFXDevice error: Invalid default state hash detected!");
    // Activate the default render states
    _previousStateBlockHash = _stateBlockMap[0].getHash();
    setStateBlock(_defaultStateBlockHash);
    // Our default render targets hold the screen buffer, depth buffer, and a
    // special, on demand,
    // down-sampled version of the depth buffer
    // Screen FB should use MSAA if available
    allocateRT(RenderTargetID::SCREEN, true);
    // We need to create all of our attachments for the default render targets
    // Start with the screen render target: Try a half float, multisampled
    // buffer (MSAA + HDR rendering if possible)
    TextureDescriptor screenDescriptor(TextureType::TEXTURE_2D_MS,
                                       GFXImageFormat::RGBA16F,
                                       GFXDataFormat::FLOAT_16);

    TextureDescriptor hiZDescriptor(TextureType::TEXTURE_2D_MS,
                                    GFXImageFormat::DEPTH_COMPONENT32F,
                                    GFXDataFormat::FLOAT_32);

    SamplerDescriptor hiZSampler;
    hiZSampler.setFilters(TextureFilter::NEAREST_MIPMAP_NEAREST);
    hiZSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    hiZSampler.toggleMipMaps(true);
    hiZDescriptor.setSampler(hiZSampler);
    hiZDescriptor.toggleAutomaticMipMapGeneration(false);

    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TextureFilter::NEAREST);
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.toggleMipMaps(false);
    screenDescriptor.setSampler(screenSampler);

    TextureDescriptor normalDescriptor(TextureType::TEXTURE_2D_MS,
                                       GFXImageFormat::RGB16F,
                                       GFXDataFormat::FLOAT_16);
    normalDescriptor.setSampler(screenSampler);
    
    // Add the attachments to the render targets
    RenderTarget& screenTarget = renderTarget(RenderTargetID::SCREEN, 0);
    screenTarget.addAttachment(screenDescriptor, RTAttachment::Type::Colour, 0);
    screenTarget.addAttachment(normalDescriptor, RTAttachment::Type::Colour, 1);
    screenTarget.addAttachment(hiZDescriptor,  RTAttachment::Type::Depth, 0);
    screenTarget.setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::DIVIDE_BLUE());
    screenTarget.setClearColour(RTAttachment::Type::Colour, 1, DefaultColours::WHITE());

    _activeRenderTarget = &screenTarget;

    // Reflection Targets
    SamplerDescriptor reflectionSampler;
    reflectionSampler.setFilters(TextureFilter::NEAREST);
    reflectionSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.toggleMipMaps(false);
    TextureDescriptor environmentDescriptor(TextureType::TEXTURE_CUBE_MAP,
                                            GFXImageFormat::RGBA16F,
                                            GFXDataFormat::FLOAT_16);
    environmentDescriptor.setSampler(reflectionSampler);

    RenderTargetHandle tempHandle;
    for (U32 i = 0; i < Config::MAX_REFLECTIVE_NODES_IN_VIEW; ++i) {
        tempHandle = allocateRT(RenderTargetID::REFLECTION, false);
        tempHandle._rt->addAttachment(environmentDescriptor, RTAttachment::Type::Colour, 0);
        tempHandle._rt->useAutoDepthBuffer(true);
        tempHandle._rt->create(Config::REFLECTION_TARGET_RESOLUTION);
        tempHandle._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());
    }

    for (U32 i = 0; i < Config::MAX_REFRACTIVE_NODES_IN_VIEW; ++i) {
        tempHandle = allocateRT(RenderTargetID::REFRACTION, false);
        tempHandle._rt->addAttachment(environmentDescriptor, RTAttachment::Type::Colour, 0);
        tempHandle._rt->useAutoDepthBuffer(true);
        tempHandle._rt->create(Config::REFRACTION_TARGET_RESOLUTION);
        tempHandle._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());
    }

    // Initialized our HierarchicalZ construction shader (takes a depth
    // attachment and down-samples it for every mip level)
    _HIZConstructProgram = CreateResource<ShaderProgram>(ResourceDescriptor("HiZConstruct"));
    _HIZCullProgram = CreateResource<ShaderProgram>(ResourceDescriptor("HiZOcclusionCull"));
    _displayShader = CreateResource<ShaderProgram>(ResourceDescriptor("display"));

    // Store our target z distances
    _gpuBlock._data._ZPlanesCombined.zw(vec2<F32>(
        ParamHandler::instance().getParam<F32>(_ID("rendering.zNear")),
        ParamHandler::instance().getParam<F32>(_ID("rendering.zFar"))));
    _gpuBlock._updated = true;

    // Register a 2D function used for previewing the depth buffer.
#ifdef _DEBUG
    add2DRenderFunction(DELEGATE_BIND(&GFXDevice::previewDepthBuffer, this), 0);
#endif

    ParamHandler::instance().setParam<bool>(_ID("rendering.previewDepthBuffer"), false);
    // If render targets ready, we initialize our post processing system
    PostFX::instance().init();

    _axisGizmo = getOrCreatePrimitive(false);
    _axisGizmo->name("GFXDeviceAxisGizmo");
    RenderStateBlock primitiveDescriptor(getRenderStateBlock(getDefaultStateBlock(true)));
    _axisGizmo->stateHash(primitiveDescriptor.getHash());

    ResourceDescriptor previewNormalsShader("fbPreview");
    previewNormalsShader.setThreadedLoading(false);
    _renderTargetDraw = CreateResource<ShaderProgram>(previewNormalsShader);
    assert(_renderTargetDraw != nullptr);

    // Create initial buffers, cameras etc for this resolution. It should match window size
    WindowManager& winMgr = Application::instance().windowManager();
    winMgr.handleWindowEvent(WindowEvent::RESOLUTION_CHANGED,
                             winMgr.getActiveWindow().getGUID(),
                             to_int(renderResolution.width),
                             to_int(renderResolution.height));
    setBaseViewport(vec4<I32>(0, 0, to_int(renderResolution.width), to_int(renderResolution.height)));

    // Everything is ready from the rendering point of view
    return ErrorCode::NO_ERR;
}

/// Revert everything that was set up in initRenderingAPI()
void GFXDevice::closeRenderingAPI() {
    DIVIDE_ASSERT(_api != nullptr,
                  "GFXDevice error: closeRenderingAPI called without init!");

    _axisGizmo->_canZombify = true;
    // Destroy our post processing system
    Console::printfn(Locale::get(_ID("STOP_POST_FX")));
    PostFX::destroyInstance();
    // Delete the renderer implementation
    Console::printfn(Locale::get(_ID("CLOSING_RENDERER")));
    // Delete our default render state blocks
    _stateBlockMap.clear();
    // Destroy all of the immediate mode emulation primitives created during runtime
    {
        WriteLock w_lock(_imInterfaceLock);
        MemoryManager::DELETE_VECTOR(_imInterfaces);
    }
    _gfxDataBuffer->destroy();
    MemoryManager::DELETE(_gfxDataBuffer);

    // Destroy all rendering passes and rendering bins
    RenderPassManager::destroyInstance();
    _rtPool.clear();
    _previewDepthMapShader.reset();
    _renderTargetDraw.reset();
    _HIZConstructProgram.reset();
    _HIZCullProgram.reset();
    _displayShader.reset();
    _imShader.reset();

    // Close the shader manager
    ShaderProgram::destroyStaticData();
    // Close the rendering API
    _api->closeRenderingAPI();

    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            GL_API::destroyInstance();
        } break;
        case RenderAPI::Direct3D: {
            DX_API::destroyInstance();
        } break;
        case RenderAPI::Vulkan: {
        } break;
        case RenderAPI::None: {
        } break;
        default: { 
        } break;
    };
}

/// After a swap buffer call, the CPU may be idle waiting for the GPU to draw to
/// the screen, so we try to do some processing
void GFXDevice::idle() {
    // Update the zPlanes if needed
    _gpuBlock._data._ZPlanesCombined.zw(vec2<F32>(
        ParamHandler::instance().getParam<F32>(_ID("rendering.zNear")),
        ParamHandler::instance().getParam<F32>(_ID("rendering.zFar"))));
    // Pass the idle call to the post processing system
    PostFX::instance().idle();
    // And to the shader manager
    ShaderProgram::idle();
}

void GFXDevice::beginFrame() {
    _api->beginFrame();
    setStateBlock(_defaultStateBlockHash);
}

void GFXDevice::endFrame(bool swapBuffers) {
    // Max number of frames before an unused primitive is recycled
    // (default: 180 - 3 seconds at 60 fps)
    static const I32 IN_MAX_FRAMES_RECYCLE_COUNT = 180;
    // Max number of frames before an unused primitive is deleted
    static const I32 IM_MAX_FRAMES_ZOMBIE_COUNT = 
        IN_MAX_FRAMES_RECYCLE_COUNT *
        IN_MAX_FRAMES_RECYCLE_COUNT;

    // Render all 2D debug info and call API specific flush function
    if (Application::instance().mainLoopActive()) {
        GFX::Scoped2DRendering scoped2D(true);
        ReadLock r_lock(_2DRenderQueueLock);
        for (std::pair<U32, DELEGATE_CBK<> >& callbackFunction : _2dRenderQueue) {
            callbackFunction.second();
        }
    }

    {
        WriteLock w_lock(_imInterfaceLock);
        // Remove dead primitives in 4 steps
        // 1) Partition the vector in 2 parts: valid objects first, zombie
        // objects second
        vectorImpl<IMPrimitive*>::iterator zombie = std::partition(
            std::begin(_imInterfaces), std::end(_imInterfaces),
            [](IMPrimitive* const priv) {
            return priv->zombieCounter() < IM_MAX_FRAMES_ZOMBIE_COUNT;
        });
        // 2) For every zombie object, free the memory it's using
        for (vectorImpl<IMPrimitive*>::iterator i = zombie;
                i != std::end(_imInterfaces); ++i) {
            MemoryManager::DELETE(*i);
        }
        // 3) Remove all the zombie objects once the memory is freed
        _imInterfaces.erase(zombie, std::end(_imInterfaces));
        // 4) Increment the zombie counter (if allowed) for the remaining primitives
        std::for_each(
            std::begin(_imInterfaces), std::end(_imInterfaces),
            [](IMPrimitive* primitive) -> void {
            if (primitive->_canZombify && primitive->inUse()) {
                // The zombie counter should always be reset on draw!
                primitive->zombieCounter(primitive->zombieCounter() + 1);
                // If the primitive wasn't used in a while, it may not be in use
                // so we should recycle it.
                if (primitive->zombieCounter() > IN_MAX_FRAMES_RECYCLE_COUNT) {
                    primitive->inUse(false);
                }
            }
        });
    }

    FRAME_COUNT++;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    FRAME_DRAW_CALLS = 0;
    
    // Activate the default render states
    setStateBlock(_defaultStateBlockHash);
    // Unbind shaders
    ShaderProgram::unbind();
    _api->endFrame(swapBuffers);
}

ErrorCode GFXDevice::createAPIInstance() {
    DIVIDE_ASSERT(_api == nullptr,
                  "GFXDevice error: initRenderingAPI called twice!");
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            _api = &GL_API::instance();
        } break;
        case RenderAPI::Direct3D: {
            _api = &DX_API::instance();
            Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
            return ErrorCode::GFX_NOT_SUPPORTED;
        } break;
        case RenderAPI::Vulkan: {
            Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
            return ErrorCode::GFX_NOT_SUPPORTED;
        } break;
        case RenderAPI::None: {
            Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
            return ErrorCode::GFX_NOT_SUPPORTED;
        } break;
        default: {
            Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
            return ErrorCode::GFX_NON_SPECIFIED;
        } break;
    };

    return ErrorCode::NO_ERR;
}

RenderTarget& GFXDevice::activeRenderTarget() {
    return *_activeRenderTarget;
}

const RenderTarget& GFXDevice::activeRenderTarget() const {
    return *_activeRenderTarget;
}
};