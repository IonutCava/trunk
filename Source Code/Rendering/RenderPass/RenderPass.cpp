#include "Headers/RenderPass.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

#include "Scenes/Headers/Scene.h"
#include "Geometry/Material/Headers/Material.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

namespace Divide {

namespace {
    Framebuffer::FramebufferTarget _noDepthClear;
    Framebuffer::FramebufferTarget _depthOnly;
};

RenderPass::RenderPass(stringImpl name, U8 sortKey, std::initializer_list<RenderStage> passStageFlags)
    : _sortKey(sortKey),
      _specialFlag(false),
      _name(name),
      _stageFlags(passStageFlags)
{
    _lastTotalBinSize = 0;

    _noDepthClear._clearDepthBufferOnBind = false;
    _noDepthClear._clearColorBuffersOnBind = true;
    _noDepthClear._drawMask.fill(true);

    _depthOnly._clearColorBuffersOnBind = true;
    _depthOnly._clearDepthBufferOnBind = true;
    _depthOnly._drawMask.fill(false);
    _depthOnly._drawMask[to_const_uint(TextureDescriptor::AttachmentType::Depth)] = true;
}

RenderPass::~RenderPass() 
{
}

void RenderPass::render(SceneRenderState& renderState, bool anaglyph) {
    GFXDevice& GFX = GFX_DEVICE;
    if (anaglyph && !GFX.anaglyphEnabled()) {
        return;
    }

    U32 idx = 0;
    for (RenderStage stageFlag : _stageFlags) {
        preRender(renderState, anaglyph, idx);
        Renderer& renderer = SceneManager::getInstance().getRenderer();

        bool refreshNodeData = idx == 0;

        // Actual render
        switch(stageFlag) {
            case RenderStage::Z_PRE_PASS:
            case RenderStage::DISPLAY: {
                renderer.render(
                    [stageFlag, refreshNodeData, idx]() {
                        SceneManager::getInstance().renderVisibleNodes(stageFlag, refreshNodeData);
                    },
                    renderState);
            } break;
            case RenderStage::SHADOW: {
                LightManager::getInstance().generateShadowMaps();
            } break;
            case RenderStage::REFLECTION: {
                const vec3<F32>& eyePos = renderState.getCameraConst().getEye();
                const vec2<F32>& zPlanes= renderState.getCameraConst().getZPlanes();
                GFX.generateCubeMap(*(GFX.getRenderTarget(GFXDevice::RenderTargetID::ENVIRONMENT)._buffer),
                                    0,
                                    eyePos,
                                    vec2<F32>(zPlanes.x, zPlanes.y * 0.25f),
                                    stageFlag);
            } break;
        };

        postRender(renderState, anaglyph, idx);
        idx++;
    }
}

bool RenderPass::preRender(SceneRenderState& renderState, bool anaglyph, U32 pass) {
    GFXDevice& GFX = GFX_DEVICE;
    GFX.setRenderStage(_stageFlags[pass]);

    Camera& currentCamera = renderState.getCameraMgr().getActiveCamera();
    // Anaglyph rendering
    if (anaglyph != currentCamera.isAnaglyph()) {
        currentCamera.setAnaglyph(anaglyph);
    }
    currentCamera.renderLookAt();

    bool bindShadowMaps = false;
    switch (_stageFlags[pass]) {
        case RenderStage::DISPLAY: {
            RenderQueue& renderQueue = RenderPassManager::getInstance().getQueue();
            _lastTotalBinSize = renderQueue.getRenderQueueStackSize();
            bindShadowMaps = true;
            GFX.occlusionCull(0);
            GFX.toggleDepthWrites(false);
            GFX.getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer->begin(_noDepthClear);
        } break;
        case RenderStage::REFLECTION: {
            bindShadowMaps = true;
        } break;
        case RenderStage::SHADOW: {
        } break;
        case RenderStage::Z_PRE_PASS: {
            GFX.getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer->begin(_depthOnly);
        } break;
    };
    
    if (bindShadowMaps) {
        LightManager::getInstance().bindShadowMaps();
    }

    return true;
}

bool RenderPass::postRender(SceneRenderState& renderState, bool anaglyph, U32 pass) {
    GFXDevice& GFX = GFX_DEVICE;

    RenderQueue& renderQueue = RenderPassManager::getInstance().getQueue();
    renderQueue.postRender(renderState, _stageFlags[pass]);

    Attorney::SceneRenderPass::debugDraw(SceneManager::getInstance().getActiveScene(), _stageFlags[pass]);

    switch (_stageFlags[pass]) {
        case RenderStage::DISPLAY:
        case RenderStage::Z_PRE_PASS: {
            GFXDevice::RenderTarget& renderTarget = GFX.getRenderTarget(GFXDevice::RenderTargetID::SCREEN);
            renderTarget._buffer->end();
            if (_stageFlags[pass] == RenderStage::Z_PRE_PASS) {
                GFX.constructHIZ();
                LightManager::getInstance().updateAndUploadLightData(renderState.getCameraConst().getEye(), GFX.getMatrix(MATRIX::VIEW));
                SceneManager::getInstance().getRenderer().preRender();
                renderTarget.cacheSettings();
            } else {
                GFX.toggleDepthWrites(true);
            }
        } break;

        default:
            break;
    };

    return true;
}

};