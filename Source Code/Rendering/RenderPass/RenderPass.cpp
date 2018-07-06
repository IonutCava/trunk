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

    // We need a proper, time-based system, to check reflection budget
    namespace ReflectionUtil {
        U32 g_reflectionBudget = 0;
#   if defined(_DEBUG)
        U32 g_reflectionBudgetLimit = 3;
#   else
        U32 g_reflectionBudgetLimit = 5;
#   endif
        inline bool isInBudget() {
            return g_reflectionBudget < g_reflectionBudgetLimit;
        }

        inline void resetBudget() {
            g_reflectionBudget = 0;
        }

        inline void updateBudget() {
            ++g_reflectionBudget;
        }
    };
};

RenderPass::RenderPass(stringImpl name, U8 sortKey, std::initializer_list<RenderStage> passStageFlags)
    : _sortKey(sortKey),
      _name(name),
      _useZPrePass(Config::USE_Z_PRE_PASS),
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

    // Disable pre-pass for HIZ debugging to be able to render "culled" nodes properly
    if (Config::DEBUG_HIZ_CULLING) {
        _useZPrePass = false;
    }
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
        Renderer& renderer = SceneManager::instance().getRenderer();

        bool refreshNodeData = idx == 0;

        // Actual render
        switch(stageFlag) {
            case RenderStage::Z_PRE_PASS:
            case RenderStage::DISPLAY: {
                renderer.render(
                    [stageFlag, refreshNodeData]() {
                        SceneManager::instance().renderVisibleNodes(stageFlag, refreshNodeData);
                    },
                    renderState);
            } break;
            case RenderStage::SHADOW: {
                LightManager::instance().generateShadowMaps();
            } break;
            case RenderStage::REFLECTION: {
                const vec2<F32>& zPlanes= renderState.getCameraConst().getZPlanes();
                // Get list of reflective nodes from the scene manager
                const RenderPassCuller::VisibleNodeList& nodeCache = 
                    SceneManager::instance().getSortedReflectiveNodes();

                // While in budget, update reflections
                ReflectionUtil::resetBudget();
                for (const RenderPassCuller::VisibleNode& node : nodeCache) {
                    if (!ReflectionUtil::isInBudget()) {
                        break;
                    }
                    SceneGraphNode_cptr nodePtr = node.second.lock();
                    RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                    PhysicsComponent* const pComp = nodePtr->get<PhysicsComponent>();
                    Attorney::RenderingCompRenderPass::updateReflection(*rComp, pComp->getPosition(), zPlanes);
                    ReflectionUtil::updateBudget();
                }

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
    GFXDevice::RenderTargetID target = anaglyph ? GFXDevice::RenderTargetID::ANAGLYPH
                                                : GFXDevice::RenderTargetID::SCREEN;
    currentCamera.renderLookAt();

    bool bindShadowMaps = false;
    switch (_stageFlags[pass]) {
        case RenderStage::DISPLAY: {
            RenderQueue& renderQueue = RenderPassManager::instance().getQueue();
            _lastTotalBinSize = renderQueue.getRenderQueueStackSize();
            bindShadowMaps = true;
            if (Config::USE_HIZ_CULLING) {
                GFX.occlusionCull(0);
            }
            if (_useZPrePass) {
                GFX.toggleDepthWrites(false);
            }
            GFX.getRenderTarget(target)._buffer->begin(_useZPrePass ? _noDepthClear : Framebuffer::defaultPolicy());
        } break;
        case RenderStage::REFLECTION: {
            bindShadowMaps = true;
        } break;
        case RenderStage::SHADOW: {
        } break;
        case RenderStage::Z_PRE_PASS: {
            GFX.getRenderTarget(target)._buffer->begin(_depthOnly);
        } break;
    };
    
    if (bindShadowMaps) {
        LightManager::instance().bindShadowMaps();
    }

    return true;
}

bool RenderPass::postRender(SceneRenderState& renderState, bool anaglyph, U32 pass) {
    GFXDevice& GFX = GFX_DEVICE;

    RenderQueue& renderQueue = RenderPassManager::instance().getQueue();
    renderQueue.postRender(renderState, _stageFlags[pass]);

    Attorney::SceneRenderPass::debugDraw(SceneManager::instance().getActiveScene(), _stageFlags[pass]);

    GFXDevice::RenderTargetID target = anaglyph ? GFXDevice::RenderTargetID::ANAGLYPH
                                                : GFXDevice::RenderTargetID::SCREEN;
    switch (_stageFlags[pass]) {
        case RenderStage::DISPLAY:
        case RenderStage::Z_PRE_PASS: {
            GFXDevice::RenderTarget& renderTarget = GFX.getRenderTarget(target);
            renderTarget._buffer->end();

            if (_stageFlags[pass] == RenderStage::Z_PRE_PASS) {
                GFX.constructHIZ();
                LightManager::instance().updateAndUploadLightData(renderState.getCameraConst().getEye(),
                                                                     GFX.getMatrix(MATRIX::VIEW));
                SceneManager::instance().getRenderer().preRender();
                renderTarget.cacheSettings();
            } else {
                if (_useZPrePass) {
                    GFX.toggleDepthWrites(true);
                }
            }
        } break;

        default:
            break;
    };

    return true;
}

};