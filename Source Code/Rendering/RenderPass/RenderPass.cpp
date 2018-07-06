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

RenderPass::RenderPass(stringImpl name, U8 sortKey, std::initializer_list<RenderStage> passStageFlags)
    : _sortKey(sortKey),
      _specialFlag(false),
      _name(name),
      _stageFlags(passStageFlags)
{
    _lastTotalBinSize = 0;
}

RenderPass::~RenderPass() 
{
}

void RenderPass::render(SceneRenderState& renderState, bool anaglyph) {
    if (anaglyph && !GFX_DEVICE.anaglyphEnabled()) {
        return;
    }

    U32 idx = 0;
    for (RenderStage stageFlag : _stageFlags) {
        preRender(renderState, anaglyph, idx);
        Renderer& renderer = SceneManager::getInstance().getRenderer();

        bool refreshNodeData = idx == 0;

        // Actual render
        switch(stageFlag) {
            default: {
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
                GFX_DEVICE.generateCubeMap(*(GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::ENVIRONMENT)),
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

    bool bindDepthMaps = false;
    switch (_stageFlags[pass]) {
        case RenderStage::DISPLAY: {
            RenderQueue& renderQueue = RenderPassManager::getInstance().getQueue();
            _lastTotalBinSize = renderQueue.getRenderQueueStackSize();
            bindDepthMaps = true;
            GFX.occlusionCull(0);
            GFXDevice::RenderTarget eyeTarget = anaglyph ? GFXDevice::RenderTarget::ANAGLYPH
                                                         : GFXDevice::RenderTarget::SCREEN;
            GFX.getRenderTarget(eyeTarget)->begin(Framebuffer::defaultPolicy());
        } break;
        case RenderStage::REFLECTION: {
            bindDepthMaps = true;
        } break;
        case RenderStage::SHADOW: {
        } break;
        case RenderStage::Z_PRE_PASS: {
            Framebuffer::FramebufferTarget depthPassPolicy;
            depthPassPolicy._drawMask = Framebuffer::FramebufferTarget::BufferMask::DEPTH;
            GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)->begin(depthPassPolicy);
        } break;
    };


    if (bindDepthMaps) {
        LightManager::getInstance().bindShadowMaps();
        GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)->bind(0, TextureDescriptor::AttachmentType::Depth);
    }

    return true;
}

bool RenderPass::postRender(SceneRenderState& renderState, bool anaglyph, U32 pass) {
    GFXDevice& GFX = GFX_DEVICE;

    RenderQueue& renderQueue = RenderPassManager::getInstance().getQueue();
    U16 renderBinCount = renderQueue.getRenderQueueBinSize();
    for (U16 i = 0; i < renderBinCount; ++i) {
        renderQueue.getBinSorted(i)->postRender(renderState, _stageFlags[pass]);
    }

    Attorney::SceneRenderPass::debugDraw(SceneManager::getInstance().getActiveScene(), _stageFlags[pass]);

    switch (_stageFlags[pass]) {
        case RenderStage::DISPLAY: {
            GFXDevice::RenderTarget eyeTarget = anaglyph ? GFXDevice::RenderTarget::ANAGLYPH
                                                         : GFXDevice::RenderTarget::SCREEN;
            GFX.getRenderTarget(eyeTarget)->end();
        } break;
        case RenderStage::REFLECTION: {
        } break;
        case RenderStage::SHADOW: {
        } break;
        case RenderStage::Z_PRE_PASS: {
            GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)->end();
            GFX.constructHIZ();
            LightManager::getInstance().updateAndUploadLightData(GFX.getMatrix(MATRIX_MODE::VIEW));
        } break;
    };

    return true;
}

};