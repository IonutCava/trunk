#include "Headers/GFXDevice.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Shaders/Headers/ShaderManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Headers/ForwardPlusRenderer.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"

namespace Divide {

/// Create a display context using the selected API and create all of the needed primitives needed for frame rendering
ErrorCode GFXDevice::initRenderingApi(const vec2<U16>& resolution, I32 argc, char **argv) {
    // Initialize the rendering API
    ErrorCode hardwareState = _api.initRenderingApi(resolution, argc, argv);
    // Validate initialization
    if (hardwareState != NO_ERR) {
        return hardwareState;
    }
    // Initialize the shader manager
    ShaderManager::getInstance().init();
    // Create an immediate mode shader used for general purpose rendering (e.g. to mimic the fixed function pipeline)
    _imShader = ShaderManager::getInstance().getDefaultShader();
    DIVIDE_ASSERT(_imShader != nullptr, "GFXDevice error: No immediate mode emulation shader available!");
    // Create a shader buffer to store the following info: ViewMatrix, ProjectionMatrix, ViewProjectionMatrix, CameraPositionVec, ViewportRec, zPlanesVec4 and ClipPlanes[MAX_CLIP_PLANES]
    // It should translate to (as seen by OpenGL) a uniform buffer without persistent mapping. (Many small updates with BufferSubData are recommended with the target usage of the buffer)
    _gfxDataBuffer = newSB(false, false);
    _gfxDataBuffer->Create(1, sizeof(GPUBlock)); 
    _gfxDataBuffer->Bind(SHADER_BUFFER_GPU_BLOCK);
    // Every visible node will first update this buffer with required data (WorldMatrix, NormalMatrix, Material properties, Bone count, etc)
    // Due to it's potentially huge size, it translates to (as seen by OpenGL) a Shader Storage Buffer that's persistently and coherently mapped
    _nodeBuffer = newSB(true);
    _nodeBuffer->Create(Config::MAX_VISIBLE_NODES, sizeof(NodeData));
    // Resize our window to the target resolution (usually, the splash screen resolution)
    changeResolution(resolution);
    // Create general purpose render state blocks
    RenderStateBlockDescriptor defaultStateDescriptor;
    _defaultStateBlockHash = getOrCreateStateBlock(defaultStateDescriptor);
    RenderStateBlockDescriptor defaultStateDescriptorNoDepth;
    defaultStateDescriptorNoDepth.setZReadWrite(false, true);
    _defaultStateNoDepthHash = getOrCreateStateBlock(defaultStateDescriptorNoDepth);
    RenderStateBlockDescriptor state2DRenderingDesc;
    state2DRenderingDesc.setCullMode(CULL_MODE_NONE);
    state2DRenderingDesc.setZReadWrite(false, true);
    _state2DRenderingHash = getOrCreateStateBlock(state2DRenderingDesc);
    RenderStateBlockDescriptor stateDepthOnlyRendering;
    stateDepthOnlyRendering.setColorWrites(false, false, false, false);
    stateDepthOnlyRendering.setZFunc(CMP_FUNC_ALWAYS);
    _stateDepthOnlyRenderingHash = getOrCreateStateBlock(stateDepthOnlyRendering);
    // Block with hash 0 is null, and it's used to force a block update, bypassing state comparison with previous blocks
    _stateBlockMap[0] = nullptr;
    // The general purpose render state blocks are both mandatory and must differ from each other at a state hash level
    DIVIDE_ASSERT(_stateDepthOnlyRenderingHash != _state2DRenderingHash,    "GFXDevice error: Invalid default state hash detected!");
    DIVIDE_ASSERT(_state2DRenderingHash        != _defaultStateNoDepthHash, "GFXDevice error: Invalid default state hash detected!");
    DIVIDE_ASSERT(_defaultStateNoDepthHash     != _defaultStateBlockHash,   "GFXDevice error: Invalid default state hash detected!");
    // Activate the default render states
    setStateBlock(_defaultStateBlockHash);
    // Our default render targets hold the screen buffer, depth buffer, and a special, on demand, down-sampled version of the depth buffer
    // Screen FB should use MSAA if available
    _renderTarget[RENDER_TARGET_SCREEN]       = newFB(true);
    // The depth buffer should probably be merged into the screen buffer
    _renderTarget[RENDER_TARGET_DEPTH]        = newFB(false);
    // We need to create all of our attachments for the default render targets
    // Start with the screen render target: Try a half float, multisampled buffer (MSAA + HDR rendering if possible)
    TextureDescriptor screenDescriptor(TEXTURE_2D_MS, RGBA16F, FLOAT_16);
    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
    screenSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    screenSampler.toggleMipMaps(false);
    screenDescriptor.setSampler(screenSampler);
    // Next, create a depth attachment for the screen render target.
    // Must also multisampled. Use full float precision for long view distances
    SamplerDescriptor depthSampler;
    depthSampler.setFilters(TEXTURE_FILTER_NEAREST);
    depthSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthSampler.toggleMipMaps(false);
    // Use greater or equal depth compare function, but depth comparison is disabled, anyway.
    depthSampler._cmpFunc = CMP_FUNC_GEQUAL; 
    TextureDescriptor depthDescriptor(TEXTURE_2D_MS, DEPTH_COMPONENT32F, FLOAT_32);
    depthDescriptor.setSampler(depthSampler);
    // The depth render target uses a HierarchicalZ buffer to help with occlusion culling
    // Must be as close as possible to the screen's depth buffer
    SamplerDescriptor depthSamplerHiZ;
    depthSamplerHiZ.setFilters(TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST, TEXTURE_FILTER_NEAREST);
    depthSamplerHiZ.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthSamplerHiZ.toggleMipMaps(true);
    TextureDescriptor depthDescriptorHiZ(TEXTURE_2D_MS, DEPTH_COMPONENT32F, FLOAT_32);
    depthDescriptorHiZ.setSampler(depthSamplerHiZ);
    /// Add the attachments to the render targets
    _renderTarget[RENDER_TARGET_SCREEN]->AddAttachment(screenDescriptor, TextureDescriptor::Color0);
    _renderTarget[RENDER_TARGET_SCREEN]->AddAttachment(depthDescriptor,  TextureDescriptor::Depth);
    _renderTarget[RENDER_TARGET_SCREEN]->Create(resolution.width, resolution.height);
    _renderTarget[RENDER_TARGET_DEPTH]->AddAttachment(depthDescriptorHiZ, TextureDescriptor::Depth);
    _renderTarget[RENDER_TARGET_DEPTH]->toggleColorWrites(false);
    _renderTarget[RENDER_TARGET_DEPTH]->Create(resolution.width, resolution.height);
    /// If we enabled anaglyph rendering, we need a second target, identical to the screen target used to render the scene at an offset
    if(_enableAnaglyph){
        _renderTarget[RENDER_TARGET_ANAGLYPH] = newFB(true);
        _renderTarget[RENDER_TARGET_ANAGLYPH]->AddAttachment(screenDescriptor, TextureDescriptor::Color0);
        _renderTarget[RENDER_TARGET_ANAGLYPH]->AddAttachment(depthDescriptor,  TextureDescriptor::Depth);
        _renderTarget[RENDER_TARGET_ANAGLYPH]->Create(resolution.width, resolution.height);
    }
    /// If render targets ready, we initialize our post processing system    
    _postFX.init(resolution);
    /// We also add a couple of useful cameras used by this class. One for rendering in 2D and one for generating cube maps
    _kernel->getCameraMgr().addNewCamera("2DRenderCamera", _2DCamera);
    _kernel->getCameraMgr().addNewCamera("_gfxCubeCamera", _cubeCamera);
    /// Initialized our HierarchicalZ construction shader (takes a depth attachment and down-samples it for every mip level)
    _HIZConstructProgram = CreateResource<ShaderProgram>(ResourceDescriptor("HiZConstruct"));
    _HIZConstructProgram->UniformTexture("LastMip", 0);
    /// Store our target z distances
    _gpuBlock._ZPlanesCombined.z = ParamHandler::getInstance().getParam<F32>("rendering.zNear");
    _gpuBlock._ZPlanesCombined.w = ParamHandler::getInstance().getParam<F32>("rendering.zFar");
    /// Create a separate loading thread that shares resources with the main rendering context
    _loaderThread = New std::thread(&GFXDevice::createLoaderThread, this);
    /// Register a 2D function used for previewing the depth buffer.
#   ifdef _DEBUG
        add2DRenderFunction(DELEGATE_BIND(&GFXDevice::previewDepthBuffer, this), 0);
#   endif
    // We start of with a forward plus renderer
    setRenderer(New ForwardPlusRenderer());
    /// Everything is ready from the rendering point of view
    return NO_ERR;
}

/// Revert everything that was set up in initRenderingAPI()
void GFXDevice::closeRenderingApi() {
    // Delete the internal shader
    RemoveResource(_HIZConstructProgram);
    // Destroy our post processing system
    PRINT_FN(Locale::get("STOP_POST_FX"));
    PostFX::destroyInstance();
    // Delete the renderer implementation
    PRINT_FN(Locale::get("CLOSING_RENDERER"));
    SAFE_DELETE(_renderer);
    // Close the rendering API
    _api.closeRenderingApi();
    // Wait for the loading thread to terminate
    _loaderThread->join();
    // And delete it
    SAFE_DELETE(_loaderThread);
    // Delete our default render state blocks
    for(RenderStateMap::value_type& it : _stateBlockMap) {
        SAFE_DELETE(it.second);
    }
    _stateBlockMap.clear();
    // Destroy all of the immediate mode emulation primitives created during runtime
    for (IMPrimitive*& priv : _imInterfaces) {
        SAFE_DELETE(priv);
    }
    _imInterfaces.clear();
    // Destroy all rendering passes and rendering bins
    RenderPassManager::destroyInstance();
    // Delete all of our rendering targets
    for (Framebuffer*& renderTarget : _renderTarget) {
        SAFE_DELETE(renderTarget);
    }
    // Delete our shader buffers
    SAFE_DELETE(_gfxDataBuffer);
    SAFE_DELETE(_nodeBuffer);
    // Close the shader manager
    _shaderManager.destroy();
}

/// After a swap buffer call, the CPU may be idle waiting for the GPU to draw to the screen, so we try to do some processing
void GFXDevice::idle() {
    // Update the zPlanes if needed
    _gpuBlock._ZPlanesCombined.z = ParamHandler::getInstance().getParam<F32>("rendering.zNear");
    _gpuBlock._ZPlanesCombined.w = ParamHandler::getInstance().getParam<F32>("rendering.zFar");
    // Pass the idle call to the post processing system
    _postFX.idle();
    // And to the shader manager
    _shaderManager.idle();
}

void GFXDevice::beginFrame() {
    _api.beginFrame();
    setStateBlock(_defaultStateBlockHash);
}

void GFXDevice::endFrame() { 
    // Max number of frames before an unused primitive is deleted (default: 180 - 3 seconds at 60 fps)
    static const I32 IM_MAX_FRAMES_ZOMBIE_COUNT = 180;

    if (Application::getInstance().mainLoopActive()) {
        // Render all 2D debug info and call API specific flush function
        toggle2D(true);
		for (std::pair<U32, DELEGATE_CBK<> >& callbackFunction : _2dRenderQueue) {
            callbackFunction.second();
        }
        toggle2D(false);

        //Remove dead primitives in 3 steps (or we could automate this with shared_ptr?):
        //1) Partition the vector in 2 parts: valid objects first, zombie objects second
        vectorImpl<IMPrimitive* >::iterator zombie = std::partition(_imInterfaces.begin(), _imInterfaces.end(),
                                                                    [](IMPrimitive* const priv){
                                                                        if(!priv->_canZombify) return true;
                                                                        return priv->zombieCounter() < IM_MAX_FRAMES_ZOMBIE_COUNT;
                                                                    });
        //2) For every zombie object, free the memory it's using
        for ( vectorImpl<IMPrimitive *>::iterator i = zombie ; i != _imInterfaces.end(); ++i ) {
            SAFE_DELETE(*i);
        }
        //3) Remove all the zombie objects once the memory is freed
        _imInterfaces.erase(zombie, _imInterfaces.end());
    
        FRAME_COUNT++;
        FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
        FRAME_DRAW_CALLS = 0;
    }

    _api.endFrame();  
}

Renderer* GFXDevice::getRenderer() const {
    DIVIDE_ASSERT(_renderer != nullptr, "GFXDevice error: Renderer requested but not created!"); 
    return _renderer;
}

void GFXDevice::setRenderer(Renderer* const renderer) {
    DIVIDE_ASSERT(renderer != nullptr, "GFXDevice error: Tried to create an invalid renderer!"); 
    SAFE_UPDATE(_renderer, renderer);
}

};