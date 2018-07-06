#include "config.h"

#include "Headers/GFXDevice.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Headers/EnvironmentProbe.h"

#include "Managers/Headers/RenderPassManager.h"

#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#include "Geometry/Material/Headers/ShaderComputeQueue.h"

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Direct3D/Headers/DXWrapper.h"

#include "RenderDoc-Manager/RenderDocManager.h"

namespace Divide {

/// Create a display context using the selected API and create all of the needed
/// primitives needed for frame rendering
ErrorCode GFXDevice::initRenderingAPI(I32 argc, char** argv, const vec2<U16>& renderResolution) {
    ErrorCode hardwareState = createAPIInstance();
    if (hardwareState == ErrorCode::NO_ERR) {
        // Initialize the rendering API
        if (Config::ENABLE_GPU_VALIDATION) {
           //_renderDocManager = 
           //   std::make_shared<RenderDocManager>(Application::instance().sysInfo()._windowHandle,
           //                                      ".\\RenderDoc\\renderdoc.dll",
           //                                      L"\\RenderDoc\\Captures\\");
        }
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

    ResourceCache& cache = parent().resourceCache();

    // Initialize the shader manager
    ShaderProgram::onStartup(cache);
    EnvironmentProbe::onStartup(*this);
    PostFX::createInstance();
    // Create a shader buffer to store the following info:
    // ViewMatrix, ProjectionMatrix, ViewProjectionMatrix, CameraPositionVec, ViewportRec, zPlanesVec4 and ClipPlanes[MAX_CLIP_PLANES]
    // It should translate to (as seen by OpenGL) a uniform buffer without persistent mapping.
    // (Many small updates with BufferSubData are recommended with the target usage of the buffer)
    _gfxDataBuffer = newSB(1, false, false, BufferUpdateFrequency::OFTEN);
    _gfxDataBuffer->create(1, sizeof(GFXShaderData::GPUData));
    _gfxDataBuffer->bind(ShaderBufferLocation::GPU_BLOCK);

    _shaderComputeQueue = MemoryManager_NEW ShaderComputeQueue(cache);

    // Utility cameras
    _2DCamera = Camera::createCamera("2DRenderCamera", Camera::CameraType::FREE_FLY);
    _cubeCamera = Camera::createCamera("_gfxCubeCamera", Camera::CameraType::FREE_FLY);
    _dualParaboloidCamera = Camera::createCamera("_gfxParaboloidCamera", Camera::CameraType::FREE_FLY);

    _2DCamera->lockView(true);

    // Create general purpose render state blocks
    RenderStateBlock::init();
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
    assert(_stateDepthOnlyRenderingHash != _state2DRenderingHash &&
           "GFXDevice error: Invalid default state hash detected!");
    assert(_state2DRenderingHash != _defaultStateNoDepthHash &&
           "GFXDevice error: Invalid default state hash detected!");
    assert(_defaultStateNoDepthHash != _defaultStateBlockHash &&
           "GFXDevice error: Invalid default state hash detected!");
    // Activate the default render states
    _api->setStateBlock(_defaultStateBlockHash);
    // Our default render targets hold the screen buffer, depth buffer, and a
    // special, on demand,
    // down-sampled version of the depth buffer
    // Screen FB should use MSAA if available
    allocateRT(RenderTargetUsage::SCREEN, "Screen");
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

    TextureDescriptor normalDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RG16F,
                                       GFXDataFormat::FLOAT_16);
    normalDescriptor.setSampler(screenSampler);
    
    TextureDescriptor velocityDescriptor(TextureType::TEXTURE_2D,
                                         GFXImageFormat::RG16F,
                                         GFXDataFormat::FLOAT_16);
    velocityDescriptor.setSampler(screenSampler);

    // Add the attachments to the render targets
    RenderTarget& screenTarget = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    screenTarget.addAttachment(screenDescriptor, RTAttachment::Type::Colour, to_const_ubyte(ScreenTargets::ALBEDO), false);
    screenTarget.addAttachment(normalDescriptor, RTAttachment::Type::Colour, to_const_ubyte(ScreenTargets::NORMALS), false);
    screenTarget.addAttachment(velocityDescriptor, RTAttachment::Type::Colour, to_const_ubyte(ScreenTargets::VELOCITY), false);
    screenTarget.addAttachment(hiZDescriptor,  RTAttachment::Type::Depth, 0, true);
    screenTarget.setClearColour(RTAttachment::Type::Colour, to_const_ubyte(ScreenTargets::ALBEDO), DefaultColours::DIVIDE_BLUE());
    screenTarget.setClearColour(RTAttachment::Type::Colour, to_const_ubyte(ScreenTargets::NORMALS), DefaultColours::WHITE());
    screenTarget.setClearColour(RTAttachment::Type::Colour, to_const_ubyte(ScreenTargets::VELOCITY), DefaultColours::WHITE());

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

    TextureDescriptor depthDescriptor(TextureType::TEXTURE_CUBE_MAP,
                                      GFXImageFormat::DEPTH_COMPONENT32F,
                                      GFXDataFormat::FLOAT_32);

    depthDescriptor.setSampler(reflectionSampler);

    RenderTargetHandle tempHandle;
    for (U32 i = 0; i < Config::MAX_REFLECTIVE_NODES_IN_VIEW; ++i) {
        tempHandle = allocateRT(RenderTargetUsage::REFLECTION, Util::StringFormat("Reflection_%d", i));
        tempHandle._rt->addAttachment(environmentDescriptor, RTAttachment::Type::Colour, 0, false);
        tempHandle._rt->addAttachment(depthDescriptor, RTAttachment::Type::Depth, 0, false);
        tempHandle._rt->create(Config::REFLECTION_TARGET_RESOLUTION);
        tempHandle._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());
    }

    for (U32 i = 0; i < Config::MAX_REFRACTIVE_NODES_IN_VIEW; ++i) {
        tempHandle = allocateRT(RenderTargetUsage::REFRACTION, Util::StringFormat("Refraction_%d", i));
        tempHandle._rt->addAttachment(environmentDescriptor, RTAttachment::Type::Colour, 0, false);
        tempHandle._rt->addAttachment(depthDescriptor, RTAttachment::Type::Depth, 0, false);
        tempHandle._rt->create(Config::REFRACTION_TARGET_RESOLUTION);
        tempHandle._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());
    }

    // Initialized our HierarchicalZ construction shader (takes a depth
    // attachment and down-samples it for every mip level)
    _HIZConstructProgram = CreateResource<ShaderProgram>(cache, ResourceDescriptor("HiZConstruct"));
    _HIZCullProgram = CreateResource<ShaderProgram>(cache, ResourceDescriptor("HiZOcclusionCull"));
    _displayShader = CreateResource<ShaderProgram>(cache, ResourceDescriptor("display"));

    ParamHandler& par = ParamHandler::instance();
    PostFX& postFX = PostFX::instance();

    // Store our target z distances
    _gpuBlock._data._ZPlanesCombined.zw(vec2<F32>(
        par.getParam<F32>(_ID("rendering.zNear")),
        par.getParam<F32>(_ID("rendering.zFar"))));
    _gpuBlock._needsUpload = true;

    // Register a 2D function used for previewing the depth buffer.
    if (Config::Build::IS_DEBUG_BUILD) {
        add2DRenderFunction(GUID_DELEGATE_CBK([this]() { previewDepthBuffer(); }), 0);
    }

    par.setParam<bool>(_ID("rendering.previewDepthBuffer"), false);
    // If render targets ready, we initialize our post processing system
    postFX.init(*this, cache);
    bool enablePostAA = par.getParam<I32>(_ID("rendering.PostAASamples"), 0) > 0;
    bool enableSSR = false;
    bool enableSSAO = par.getParam<bool>(_ID("postProcessing.enableSSAO"), false);
    bool enableDoF = par.getParam<bool>(_ID("postProcessing.enableDepthOfField"), false);
    bool enableMotionBlur = false;
    bool enableBloom = par.getParam<bool>(_ID("postProcessing.enableBloom"), false);
    bool enableLUT = false;

    if (enablePostAA) {
        postFX.pushFilter(FilterType::FILTER_SS_ANTIALIASING);
    }
    if (enableSSR) {
        postFX.pushFilter(FilterType::FILTER_SS_REFLECTIONS);
    }
    if (enableSSAO) {
        postFX.pushFilter(FilterType::FILTER_SS_AMBIENT_OCCLUSION);
    }
    if (enableDoF) {
        postFX.pushFilter(FilterType::FILTER_DEPTH_OF_FIELD);
    }
    if (enableMotionBlur) {
        postFX.pushFilter(FilterType::FILTER_MOTION_BLUR);
    }
    if (enableBloom) {
        postFX.pushFilter(FilterType::FILTER_BLOOM);
    }
    if (enableLUT) {
        postFX.pushFilter(FilterType::FILTER_LUT_CORECTION);
    }

    _axisGizmo = newIMP();
    _axisGizmo->name("GFXDeviceAxisGizmo");
    RenderStateBlock primitiveDescriptor(RenderStateBlock::get(getDefaultStateBlock(true)));
    _axisGizmo->stateHash(primitiveDescriptor.getHash());

    _debugFrustumPrimitive = newIMP();
    _debugFrustumPrimitive->name("DebugFrustum");
    _debugFrustumPrimitive->stateHash(primitiveDescriptor.getHash());

    ResourceDescriptor previewNormalsShader("fbPreview");
    previewNormalsShader.setThreadedLoading(false);
    _renderTargetDraw = CreateResource<ShaderProgram>(cache, previewNormalsShader);
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
    assert(_api != nullptr && "GFXDevice error: closeRenderingAPI called without init!");
    _axisGizmo->clear();
    _debugFrustumPrimitive->clear();
    // Destroy our post processing system
    Console::printfn(Locale::get(_ID("STOP_POST_FX")));
    PostFX::destroyInstance();
    // Delete the renderer implementation
    Console::printfn(Locale::get(_ID("CLOSING_RENDERER")));
    RenderStateBlock::clear();

    EnvironmentProbe::onShutdown(*this);
    _rtPool.clear();

    _previewDepthMapShader = nullptr;
    _renderTargetDraw = nullptr;
    _HIZConstructProgram = nullptr;
    _HIZCullProgram = nullptr;
    _displayShader = nullptr;

    MemoryManager::DELETE(_renderer);

    // Close the shader manager
    MemoryManager::DELETE(_shaderComputeQueue);
    ShaderProgram::onShutdown();
    _gpuObjectArena.clear();
    // Close the rendering API
    _api->closeRenderingAPI();
    _api.release();
    if (!_graphicResources.empty()) {
        Console::errorfn(Locale::get(_ID("ERROR_GFX_LEAKED_RESOURCES")), _graphicResources.size());
    }
}

/// After a swap buffer call, the CPU may be idle waiting for the GPU to draw to
/// the screen, so we try to do some processing
void GFXDevice::idle() {
    static const Task idleTask;
    // Update the zPlanes if needed
    _gpuBlock._data._ZPlanesCombined.zw(vec2<F32>(
        ParamHandler::instance().getParam<F32>(_ID("rendering.zNear")),
        ParamHandler::instance().getParam<F32>(_ID("rendering.zFar"))));
    _shaderComputeQueue->idle();
    // Pass the idle call to the post processing system
    PostFX::instance().idle();
    // And to the shader manager
    ShaderProgram::idle();

    UpgradableReadLock r_lock(_GFXLoadQueueLock);
    if (!_GFXLoadQueue.empty()) {
        for(DELEGATE_CBK_PARAM<const Task&>& cbk : _GFXLoadQueue) {
            cbk(idleTask);
        }
        UpgradeToWriteLock w_lock(r_lock);
        _GFXLoadQueue.clear();
    }
}

void GFXDevice::beginFrame() {
    if (Config::ENABLE_GPU_VALIDATION) {
        if (_renderDocManager) {
            _renderDocManager->StartFrameCapture();
        }
    }

    _api->beginFrame();
    _api->setStateBlock(_defaultStateBlockHash);
}

void GFXDevice::endFrame(bool swapBuffers) {
    FRAME_COUNT++;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    FRAME_DRAW_CALLS = 0;
    
    // Activate the default render states
    _api->setStateBlock(_defaultStateBlockHash);
    // Unbind shaders
    ShaderProgram::unbind();
    _api->endFrame(swapBuffers);

    if (Config::ENABLE_GPU_VALIDATION) {
        if (_renderDocManager) {
            _renderDocManager->EndFrameCapture();
        }
    }
}

ErrorCode GFXDevice::createAPIInstance() {
    assert(_api == nullptr && "GFXDevice error: initRenderingAPI called twice!");

    ErrorCode err = ErrorCode::NO_ERR;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            _api = std::make_unique<GL_API>(*this);
        } break;
        case RenderAPI::Direct3D: {
            _api = std::make_unique<DX_API>(*this);
        } break;

        default:
        case RenderAPI::None:
        case RenderAPI::Vulkan: {
            err = ErrorCode::GFX_NON_SPECIFIED;
        } break;
    };

    DIVIDE_ASSERT(_api != nullptr, Locale::get(_ID("ERROR_GFX_DEVICE_API")));
    return err;
}

RenderTarget& GFXDevice::activeRenderTarget() {
    return *_activeRenderTarget;
}

const RenderTarget& GFXDevice::activeRenderTarget() const {
    return *_activeRenderTarget;
}
};