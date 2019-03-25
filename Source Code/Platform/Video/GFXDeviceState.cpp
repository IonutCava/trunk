#include "stdafx.h"

#include "config.h"

#include "Headers/GFXDevice.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
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

#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/RenderBackend/Vulkan/Headers/VKWrapper.h"
#include "Platform/Video/RenderBackend/None/Headers/NoneWrapper.h"

#include "RenderDoc-Manager/RenderDocManager.h"

namespace Divide {

/// Create a display context using the selected API and create all of the needed
/// primitives needed for frame rendering
ErrorCode GFXDevice::initRenderingAPI(I32 argc, char** argv, const vec2<U16>& renderResolution) {
    ErrorCode hardwareState = createAPIInstance();
    Configuration& config = _parent.platformContext().config();

    if (hardwareState == ErrorCode::NO_ERR) {
        // Initialize the rendering API
        if (Config::ENABLE_GPU_VALIDATION) {
           //_renderDocManager = 
           //   std::make_shared<RenderDocManager>(const_sysInfo()._windowHandle,
           //                                      ".\\RenderDoc\\renderdoc.dll",
           //                                      L"\\RenderDoc\\Captures\\");
        }
        hardwareState = _api->initRenderingAPI(argc, argv, config);
    }

    if (hardwareState != ErrorCode::NO_ERR) {
        // Validate initialization
        return hardwareState;
    }

    stringImpl refreshRates;
    vec_size displayCount = gpuState().getDisplayCount();
    for (vec_size idx = 0; idx < displayCount; ++idx) {
        const vector<GPUState::GPUVideoMode>& registeredModes = gpuState().getDisplayModes(idx);
        Console::printfn(Locale::get(_ID("AVAILABLE_VIDEO_MODES")), idx, registeredModes.size());

        for (const GPUState::GPUVideoMode& mode : registeredModes) {
            // Optionally, output to console/file each display mode
            refreshRates = Util::StringFormat("%d", mode._refreshRate.front());
            vec_size refreshRateCount = mode._refreshRate.size();
            for (vec_size i = 1; i < refreshRateCount; ++i) {
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
    _rtPool = MemoryManager_NEW GFXRTPool(*this);

    // Initialize the shader manager
    ShaderProgram::onStartup(*this, cache);
    EnvironmentProbe::onStartup(*this);
    // Create a shader buffer to store the GFX rendering info (matrices, options, etc)
    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._elementCount = 1;
    bufferDescriptor._elementSize = sizeof(GFXShaderData::GPUData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
    bufferDescriptor._initialData = &_gpuBlock._data;
    bufferDescriptor._name = "DVD_GPU_DATA";
    _gfxDataBuffer = newSB(bufferDescriptor);

    _shaderComputeQueue = MemoryManager_NEW ShaderComputeQueue(cache);

    // Create general purpose render state blocks
    RenderStateBlock::init();
    RenderStateBlock defaultState;
    _defaultStateBlockHash = defaultState.getHash();

    RenderStateBlock defaultStateNoDepth;
    defaultStateNoDepth.depthTestEnabled(false);
    _defaultStateNoDepthHash = defaultStateNoDepth.getHash();

    RenderStateBlock state2DRendering;
    state2DRendering.setCullMode(CullMode::NONE);
    state2DRendering.depthTestEnabled(false);
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

    // We need to create all of our attachments for the default render targets
    // Start with the screen render target: Try a half float, multisampled
    // buffer (MSAA + HDR rendering if possible)

    U8 msaaSamples = config.rendering.msaaSamples;

    TextureDescriptor screenDescriptor(TextureType::TEXTURE_2D_MS, GFXImageFormat::RGB, GFXDataFormat::FLOAT_16);
    TextureDescriptor normalAndVelocityDescriptor(TextureType::TEXTURE_2D_MS, GFXImageFormat::RGBA, GFXDataFormat::FLOAT_16);
    TextureDescriptor depthDescriptor(TextureType::TEXTURE_2D_MS, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::FLOAT_32);

    SamplerDescriptor defaultSampler = {};
    defaultSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    defaultSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    defaultSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    defaultSampler._minFilter = TextureFilter::NEAREST;
    defaultSampler._magFilter = TextureFilter::NEAREST;

    screenDescriptor.setSampler(defaultSampler);
    screenDescriptor.msaaSamples(msaaSamples);

    depthDescriptor.setSampler(defaultSampler);
    depthDescriptor.msaaSamples(msaaSamples);

    normalAndVelocityDescriptor.setSampler(defaultSampler);
    normalAndVelocityDescriptor.msaaSamples(msaaSamples);

    {
        vector<RTAttachmentDescriptor> attachments = {
            { screenDescriptor,              RTAttachmentType::Colour, to_U8(ScreenTargets::ALBEDO), DefaultColours::DIVIDE_BLUE },
            { normalAndVelocityDescriptor,   RTAttachmentType::Colour, to_U8(ScreenTargets::NORMALS_AND_VELOCITY), FColour(0.0f, 0.0f, 0.0f, 0.0f) },
            { depthDescriptor,               RTAttachmentType::Depth }
        };

        RenderTargetDescriptor screenDesc = {};
        screenDesc._name = "Screen";
        screenDesc._resolution = renderResolution;
        screenDesc._attachmentCount = to_U8(attachments.size());
        screenDesc._attachments = attachments.data();

        // Our default render targets hold the screen buffer, depth buffer, and a special, on demand,
        // down-sampled version of the depth buffer
        // Screen FB should use MSAA if available
        _rtPool->allocateRT(RenderTargetUsage::SCREEN, screenDesc);
    }

    U16 reflectRes = std::max(renderResolution.width, renderResolution.height) / Config::REFLECTION_TARGET_RESOLUTION_DOWNSCALE_FACTOR;

    ResourceDescriptor prevDepthTex("PREV_DEPTH");
    depthDescriptor.msaaSamples(0);
    prevDepthTex.setPropertyDescriptor(depthDescriptor);
    prevDepthTex.setThreadedLoading(false);
    _prevDepthBuffer = CreateResource<Texture>(parent().resourceCache(), prevDepthTex);
    assert(_prevDepthBuffer);
    Texture::TextureLoadInfo info;
    _prevDepthBuffer->loadData(info, NULL, renderResolution);

    TextureDescriptor hiZDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::FLOAT_32);
    SamplerDescriptor hiZSampler = {};
    hiZSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    hiZSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    hiZSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    hiZSampler._minFilter = TextureFilter::NEAREST_MIPMAP_NEAREST;
    hiZSampler._magFilter = TextureFilter::NEAREST;

    hiZDescriptor.setSampler(hiZSampler);
    hiZDescriptor.automaticMipMapGeneration(false);

    vector<RTAttachmentDescriptor> hiZAttachments = {
        { hiZDescriptor, RTAttachmentType::Depth }
    };

    {
        RenderTargetDescriptor hizRTDesc = {};
        hizRTDesc._name = "HiZ";
        hizRTDesc._resolution = renderResolution;
        hizRTDesc._attachmentCount = to_U8(hiZAttachments.size());
        hizRTDesc._attachments = hiZAttachments.data();

        _rtPool->allocateRT(RenderTargetUsage::HI_Z, hizRTDesc);
        hizRTDesc._resolution = reflectRes;
        _rtPool->allocateRT(RenderTargetUsage::HI_Z_REFLECT, hizRTDesc);
    }

    if (Config::Build::ENABLE_EDITOR) {
        SamplerDescriptor editorSampler = {};
        editorSampler._minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
        editorSampler._magFilter = TextureFilter::LINEAR;
        editorSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
        editorSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
        editorSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
        editorSampler._anisotropyLevel = 0;

        TextureDescriptor editorDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGB, GFXDataFormat::UNSIGNED_BYTE);
        editorDescriptor.setSampler(editorSampler);

        vector<RTAttachmentDescriptor> attachments = {
            { editorDescriptor, RTAttachmentType::Colour, to_U8(ScreenTargets::ALBEDO), DefaultColours::DIVIDE_BLUE }
        };

        RenderTargetDescriptor editorDesc = {};
        editorDesc._name = "Editor";
        editorDesc._resolution = renderResolution;
        editorDesc._attachmentCount = to_U8(attachments.size());
        editorDesc._attachments = attachments.data();
        _rtPool->allocateRT(RenderTargetUsage::EDITOR, editorDesc);
    }

    {
        SamplerDescriptor accumulationSampler = {};
        accumulationSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
        accumulationSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
        accumulationSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
        accumulationSampler._minFilter = TextureFilter::NEAREST;
        accumulationSampler._magFilter = TextureFilter::NEAREST;

        TextureDescriptor accumulationDescriptor(TextureType::TEXTURE_2D_MS, GFXImageFormat::RGBA, GFXDataFormat::FLOAT_16);
        accumulationDescriptor.msaaSamples(msaaSamples);
        accumulationDescriptor.automaticMipMapGeneration(false);
        accumulationDescriptor.setSampler(accumulationSampler);

        TextureDescriptor revealageDescriptor(TextureType::TEXTURE_2D_MS, GFXImageFormat::RED, GFXDataFormat::FLOAT_16);
        revealageDescriptor.msaaSamples(msaaSamples);
        revealageDescriptor.automaticMipMapGeneration(false);
        revealageDescriptor.setSampler(accumulationSampler);

        vector<RTAttachmentDescriptor> attachments = {
            { accumulationDescriptor, RTAttachmentType::Colour, to_U8(ScreenTargets::ACCUMULATION), FColour(0.0f, 0.0f, 0.0f, 0.0f) },
            { revealageDescriptor, RTAttachmentType::Colour, to_U8(ScreenTargets::REVEALAGE), DefaultColours::WHITE}
        };

        const RTAttachment_ptr& screenAttachment = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachmentPtr(RTAttachmentType::Colour, to_U8(ScreenTargets::ALBEDO));
        const RTAttachment_ptr& normalsAttachment = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachmentPtr(RTAttachmentType::Colour, to_U8(ScreenTargets::NORMALS_AND_VELOCITY));
        const RTAttachment_ptr& screenDepthAttachment = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachmentPtr(RTAttachmentType::Depth, 0);

        vector<ExternalRTAttachmentDescriptor> externalAttachments = {
            { normalsAttachment,  RTAttachmentType::Colour, to_U8(ScreenTargets::EXTRA) },
            { screenAttachment,  RTAttachmentType::Colour, to_U8(ScreenTargets::MODULATE) },
            { screenDepthAttachment,  RTAttachmentType::Depth }
        };

        RenderTargetDescriptor oitDesc = {};
        oitDesc._name = "OIT_FULL_RES";
        oitDesc._resolution = renderResolution;
        oitDesc._attachmentCount = to_U8(attachments.size());
        oitDesc._attachments = attachments.data();
        oitDesc._externalAttachmentCount = to_U8(externalAttachments.size());
        oitDesc._externalAttachments = externalAttachments.data();
        _rtPool->allocateRT(RenderTargetUsage::OIT, oitDesc);
    }

    // Reflection Targets
    SamplerDescriptor reflectionSampler = {};
    reflectionSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    reflectionSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    reflectionSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    reflectionSampler._minFilter = TextureFilter::NEAREST;
    reflectionSampler._magFilter = TextureFilter::NEAREST;

    RenderTargetHandle tempHandle;

    {
        // A could be used for anything. E.G. depth
        TextureDescriptor environmentDescriptorPlanar(TextureType::TEXTURE_2D, GFXImageFormat::RGBA, GFXDataFormat::UNSIGNED_BYTE);
        environmentDescriptorPlanar.setSampler(reflectionSampler);

        TextureDescriptor depthDescriptorPlanar(TextureType::TEXTURE_2D, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::FLOAT_32);
        depthDescriptorPlanar.setSampler(reflectionSampler);

        RenderTargetDescriptor hizRTDesc = {};
        hizRTDesc._resolution = reflectRes;
        hizRTDesc._attachmentCount = to_U8(hiZAttachments.size());
        hizRTDesc._attachments = hiZAttachments.data();

        {
            vector<RTAttachmentDescriptor> attachments = {
                { environmentDescriptorPlanar, RTAttachmentType::Colour },
                { depthDescriptorPlanar, RTAttachmentType::Depth },
            };

            RenderTargetDescriptor refDesc = {};
            refDesc._resolution = vec2<U16>(reflectRes);
            refDesc._attachmentCount = to_U8(attachments.size());
            refDesc._attachments = attachments.data();

            for (U32 i = 0; i < Config::MAX_REFLECTIVE_NODES_IN_VIEW; ++i) {
                refDesc._name = Util::StringFormat("Reflection_Planar_%d", i);
                tempHandle = _rtPool->allocateRT(RenderTargetUsage::REFLECTION_PLANAR, refDesc);
            }

            for (U32 i = 0; i < Config::MAX_REFRACTIVE_NODES_IN_VIEW; ++i) {
                refDesc._name = Util::StringFormat("Refraction_Planar_%d", i);
                tempHandle = _rtPool->allocateRT(RenderTargetUsage::REFRACTION_PLANAR, refDesc);
            }

        }
    }
    {
        TextureDescriptor environmentDescriptorCube(TextureType::TEXTURE_CUBE_ARRAY, GFXImageFormat::RGBA, GFXDataFormat::UNSIGNED_BYTE);
        environmentDescriptorCube.setSampler(reflectionSampler);

        TextureDescriptor hiZDescriptorCube(TextureType::TEXTURE_CUBE_ARRAY, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::FLOAT_32);
        hiZDescriptorCube.setSampler(hiZSampler);
        hiZDescriptorCube.automaticMipMapGeneration(false);

        TextureDescriptor depthDescriptorCube(TextureType::TEXTURE_CUBE_ARRAY, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::FLOAT_32);
        depthDescriptorCube.setSampler(reflectionSampler);

        
        vector<RTAttachmentDescriptor> attachments = {
            { environmentDescriptorCube, RTAttachmentType::Colour },
            { depthDescriptorCube, RTAttachmentType::Depth },
        };

        RenderTargetDescriptor refDesc = {};
        refDesc._resolution = vec2<U16>(reflectRes);
        refDesc._attachmentCount = to_U8(attachments.size());
        refDesc._attachments = attachments.data();

        refDesc._name = "Reflection_Cube_Array";
        tempHandle = _rtPool->allocateRT(RenderTargetUsage::REFLECTION_CUBE, refDesc);

        refDesc._name = "Refraction_Cube_Array";
        tempHandle = _rtPool->allocateRT(RenderTargetUsage::REFRACTION_CUBE, refDesc);
    }

    return ErrorCode::NO_ERR;
}

ErrorCode GFXDevice::postInitRenderingAPI() {
    ResourceCache& cache = parent().resourceCache();
    Configuration& config = _parent.platformContext().config();

    ResourceDescriptor previewNormalsShader("fbPreview");
    previewNormalsShader.setThreadedLoading(false);
    _renderTargetDraw = CreateResource<ShaderProgram>(cache, previewNormalsShader);
    assert(_renderTargetDraw != nullptr);

    ResourceDescriptor immediateModeShader("ImmediateModeEmulation.GUI");
    immediateModeShader.setThreadedLoading(false);
    _textRenderShader = CreateResource<ShaderProgram>(cache, immediateModeShader);
    assert(_textRenderShader != nullptr);

    PipelineDescriptor descriptor = {};
    descriptor._shaderProgramHandle = _textRenderShader->getID();
    descriptor._stateHash = get2DStateBlock();
    _textRenderPipeline = newPipeline(descriptor);

    // Initialized our HierarchicalZ construction shader (takes a depth
    // attachment and down-samples it for every mip level)
    ResourceDescriptor descriptor1("HiZConstruct");
    _HIZConstructProgram = CreateResource<ShaderProgram>(cache, descriptor1);
    ResourceDescriptor descriptor2("HiZOcclusionCull");
    _HIZCullProgram = CreateResource<ShaderProgram>(cache, descriptor2);
    ResourceDescriptor descriptor3("display");
    _displayShader = CreateResource<ShaderProgram>(cache, descriptor3);

    ParamHandler::instance().setParam<bool>(_ID("rendering.previewDebugViews"), false);
    // If render targets ready, we initialize our post processing system
    _postFX = std::make_unique<PostFX>(*this, cache);
    if (config.rendering.postFX.postAASamples > 0) {
        _postFX->pushFilter(FilterType::FILTER_SS_ANTIALIASING);
    }
    if (false) {
        _postFX->pushFilter(FilterType::FILTER_SS_REFLECTIONS);
    }
    if (config.rendering.postFX.enableSSAO) {
        _postFX->pushFilter(FilterType::FILTER_SS_AMBIENT_OCCLUSION);
    }
    if (config.rendering.postFX.enableDepthOfField) {
        _postFX->pushFilter(FilterType::FILTER_DEPTH_OF_FIELD);
    }
    if (false) {
        _postFX->pushFilter(FilterType::FILTER_MOTION_BLUR);
    }
    if (config.rendering.postFX.enableBloom) {
        _postFX->pushFilter(FilterType::FILTER_BLOOM);
    }
    if (false) {
        _postFX->pushFilter(FilterType::FILTER_LUT_CORECTION);
    }

    PipelineDescriptor pipelineDesc;

    _axisGizmo = newIMP();
    _axisGizmo->name("GFXDeviceAxisGizmo");
    RenderStateBlock primitiveDescriptor(RenderStateBlock::get(getDefaultStateBlock(true)));
    pipelineDesc._stateHash = primitiveDescriptor.getHash();
    pipelineDesc._shaderProgramHandle = ShaderProgram::defaultShader()->getID();

    Pipeline* primitivePipeline = newPipeline(pipelineDesc);
    _axisGizmo->pipeline(*primitivePipeline);

    _debugFrustumPrimitive = newIMP();
    _debugFrustumPrimitive->name("DebugFrustum");
    _debugFrustumPrimitive->pipeline(*primitivePipeline);

    SizeChangeParams params;
    params.width = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getWidth();
    params.height = _rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getHeight();
    params.isWindowResize = false;
    params.winGUID = context().app().windowManager().getMainWindow().getGUID();
    context().app().onSizeChange(params);

    _renderer = std::make_unique<Renderer>(context(), cache);

    // Everything is ready from the rendering point of view
    return ErrorCode::NO_ERR;
}

/// Revert everything that was set up in initRenderingAPI()
void GFXDevice::closeRenderingAPI() {
    assert(_api != nullptr && "GFXDevice error: closeRenderingAPI called without init!");
    if (_axisGizmo) {
        _axisGizmo->clear();
        _debugFrustumPrimitive->clear();
        _debugViews.clear();
    }

    // Destroy our post processing system
    Console::printfn(Locale::get(_ID("STOP_POST_FX")));
    _postFX.reset(nullptr);
    // Delete the renderer implementation
    Console::printfn(Locale::get(_ID("CLOSING_RENDERER")));
    RenderStateBlock::clear();

    EnvironmentProbe::onShutdown(*this);
    MemoryManager::SAFE_DELETE(_rtPool);

    _previewDepthMapShader = nullptr;
    _renderTargetDraw = nullptr;
    _HIZConstructProgram = nullptr;
    _HIZCullProgram = nullptr;
    _displayShader = nullptr;
    _textRenderShader = nullptr;
    _prevDepthBuffer = nullptr;

    _renderer.reset(nullptr);

    // Close the shader manager
    MemoryManager::DELETE(_shaderComputeQueue);
    ShaderProgram::onShutdown();
    _gpuObjectArena.clear();
    // Close the rendering API
    _api->closeRenderingAPI();
    _api.reset();

    UniqueLock lock(_graphicsResourceMutex);
    if (!_graphicResources.empty()) {
        stringImpl list = " [ ";
        for (const std::pair<GraphicsResource::Type, I64>& res : _graphicResources) {
            list += to_stringImpl(to_base(res.first)) + " _ " + to_stringImpl(res.second) + " ";
        }
        list += " ]";
        Console::errorfn(Locale::get(_ID("ERROR_GFX_LEAKED_RESOURCES")), _graphicResources.size());
        Console::errorfn(list.c_str());
    }
}

/// After a swap buffer call, the CPU may be idle waiting for the GPU to draw to
/// the screen, so we try to do some processing
void GFXDevice::idle() {
    _api->idle();

    _shaderComputeQueue->idle();
    // Pass the idle call to the post processing system
    postFX().idle(_parent.platformContext().config());
    // And to the shader manager
    ShaderProgram::idle();
}

void GFXDevice::beginFrame(DisplayWindow& window, bool global) {
    if (global && Config::ENABLE_GPU_VALIDATION) {
        if (_renderDocManager) {
            _renderDocManager->StartFrameCapture();
        }
    }

    if (global && _resolutionChangeQueued.second) {
        SizeChangeParams params;
        params.isWindowResize = false;
        params.isFullScreen = window.fullscreen();
        params.width = _resolutionChangeQueued.first.width;
        params.height = _resolutionChangeQueued.first.height;
        params.winGUID = context().activeWindow().getGUID();

        context().app().onSizeChange(params);
        _resolutionChangeQueued.second = false;
    }

    const vec2<U16>& drawableSize = window.getDrawableSize();
    setViewport(Rect<I32>(0, 0, drawableSize.width, drawableSize.height));

    _api->beginFrame(window, global);
    _api->setStateBlock(_defaultStateBlockHash);
}

void GFXDevice::endFrame(DisplayWindow& window, bool global) {
    if (global) {
        FRAME_COUNT++;
        FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
        FRAME_DRAW_CALLS = 0;
    }

    // Activate the default render states
    _api->endFrame(window, global);

    if (global && Config::ENABLE_GPU_VALIDATION) {
        if (_renderDocManager) {
            _renderDocManager->EndFrameCapture();
        }
    }
}

void GFXDevice::onPlayerPass(PlayerIndex index) {
    getPrevDepthBuffer()->copy(_rtPool->renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture());
}

ErrorCode GFXDevice::createAPIInstance() {
    assert(_api == nullptr && "GFXDevice error: initRenderingAPI called twice!");

    ErrorCode err = ErrorCode::NO_ERR;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            _api = std::make_unique<GL_API>(*this, _API_ID == RenderAPI::OpenGLES);
        } break;
        case RenderAPI::Vulkan: {
            _api = std::make_unique<VK_API>(*this);
        } break;
        case RenderAPI::None: {
            _api = std::make_unique<NONE_API>(*this);
        } break;
        default:
            err = ErrorCode::GFX_NON_SPECIFIED;
            break;
    };

    DIVIDE_ASSERT(_api != nullptr, Locale::get(_ID("ERROR_GFX_DEVICE_API")));
    return err;
}

};