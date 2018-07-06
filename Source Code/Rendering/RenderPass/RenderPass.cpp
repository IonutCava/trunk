#include "config.h"

#include "Headers/RenderPass.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Scenes/Headers/Scene.h"
#include "Geometry/Material/Headers/Material.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    RTDrawDescriptor _noDepthClear;
    RTDrawDescriptor _depthOnly;

    // We need a proper, time-based system, to check reflection budget
    namespace ReflectionUtil {
        U32 g_reflectionBudget = 0;

        inline bool isInBudget() {
            return g_reflectionBudget < 
                Config::MAX_REFLECTIVE_NODES_IN_VIEW;
        }

        inline void resetBudget() {
            g_reflectionBudget = 0;
        }

        inline void updateBudget() {
            ++g_reflectionBudget;
        }
        inline U32 currentEntry() {
            return g_reflectionBudget;
        }
    };
    namespace RefractionUtil {
        U32 g_refractionBudget = 0;

        inline bool isInBudget() {
            return g_refractionBudget <
                Config::MAX_REFRACTIVE_NODES_IN_VIEW;
        }

        inline void resetBudget() {
            g_refractionBudget = 0;
        }

        inline void updateBudget() {
            ++g_refractionBudget;
        }
        inline U32 currentEntry() {
            return g_refractionBudget;
        }
    };
    
};

RenderPass::BufferData::BufferData()
  : _lastCommandCount(0),
    _lasNodeCount(0)
{
    _renderData = GFX_DEVICE.newSB(1, true, true, BufferUpdateFrequency::OFTEN);
    _renderData->create(to_uint(Config::MAX_VISIBLE_NODES), sizeof(GFXDevice::NodeData));

    _cmdBuffer = GFX_DEVICE.newSB(1, true, false, BufferUpdateFrequency::OFTEN);
    _cmdBuffer->create(Config::MAX_VISIBLE_NODES, sizeof(IndirectDrawCommand));
    _cmdBuffer->addAtomicCounter(3);
}

RenderPass::BufferData::~BufferData()
{
    _cmdBuffer->destroy();
    MemoryManager::DELETE(_cmdBuffer);
    _renderData->destroy();
    MemoryManager::DELETE(_renderData);
}

RenderPass::BufferDataPool::BufferDataPool(U32 maxPasses, U32 maxStages)
{
    // Every visible node will first update this buffer with required data (WorldMatrix, NormalMatrix, Material properties, Bone count, etc)
    // Due to it's potentially huge size, it translates to (as seen by OpenGL) a Shader Storage Buffer that's persistently and coherently mapped
    // We make sure the buffer is large enough to hold data for all of our rendering stages to minimize the number of writes per frame
    // Create a shader buffer to hold all of our indirect draw commands
    // Useful if we need access to the buffer in GLSL/Compute programs
    for (U32 i = 0; i < maxPasses; ++i) {
        _buffers.emplace_back();
        BufferPool& pool = _buffers.back();

        for (U32 j = 0; j < maxStages; ++j) {
            BufferData* data = MemoryManager_NEW BufferData();
            pool.push_back(data);
        }
    }
}

RenderPass::BufferDataPool::~BufferDataPool()
{
    for (BufferPool& pool : _buffers) {
        for (BufferData* buffer : pool) {
            MemoryManager::DELETE(buffer);
        }
    }
}

RenderPass::BufferData& RenderPass::BufferDataPool::getBufferData(U32 pass, I32 idx) {
    return *_buffers[pass][idx];
}

RenderPass::RenderPass(stringImpl name, U8 sortKey, std::initializer_list<RenderStage> passStageFlags)
    : _sortKey(sortKey),
      _name(name),
      _useZPrePass(Config::USE_Z_PRE_PASS),
      _stageFlags(passStageFlags)
{
    _lastTotalBinSize = 0;

    _noDepthClear._clearDepthBufferOnBind = false;
    _noDepthClear._clearColourBuffersOnBind = true;
    _noDepthClear._drawMask.enableAll();

    _depthOnly._clearColourBuffersOnBind = true;
    _depthOnly._clearDepthBufferOnBind = true;
    _depthOnly._drawMask.disableAll();
    _depthOnly._drawMask.enabled(RTAttachment::Type::Depth, 0);

    // Disable pre-pass for HIZ debugging to be able to render "culled" nodes properly
    if (Config::DEBUG_HIZ_CULLING) {
        _useZPrePass = false;
    }

    std::pair<U32, U32> renderPassInfo = getRenderPassInfoForStages(_stageFlags);
    _passBuffers = MemoryManager_NEW BufferDataPool(renderPassInfo.first, renderPassInfo.second);
}

RenderPass::~RenderPass() 
{
    MemoryManager::DELETE(_passBuffers);
}

RenderPass::BufferData&  RenderPass::getBufferData(U32 pass, I32 idx) {
    return _passBuffers->getBufferData(pass, idx);
}

void RenderPass::render(SceneRenderState& renderState) {
    SceneManager& mgr = SceneManager::instance();
    
    U32 idx = 0;
    for (RenderStage stageFlag : _stageFlags) {
        preRender(renderState, idx);
        Renderer& renderer = mgr.getRenderer();

        bool refreshNodeData = idx == 0;

        // Actual render
        switch(stageFlag) {
            case RenderStage::Z_PRE_PASS:
            case RenderStage::DISPLAY: {
                renderer.render(
                    [stageFlag, refreshNodeData, &mgr]() {
                        mgr.renderVisibleNodes(stageFlag, refreshNodeData);
                    },
                    renderState);
            } break;
            case RenderStage::SHADOW: {
                Attorney::SceneManagerRenderPass::generateShadowMaps(mgr);
            } break;
            case RenderStage::REFLECTION: {
                // Get list of reflective and refractive nodes from the scene manager
                const RenderPassCuller::VisibleNodeList& nodeCache = mgr.getSortedReflectiveNodes();

                // While in budget, update reflections
                ReflectionUtil::resetBudget();
                for (const RenderPassCuller::VisibleNode& node : nodeCache) {
                    SceneGraphNode_cptr nodePtr = node.second.lock();
                    RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                    if (ReflectionUtil::isInBudget()) {
                        Attorney::SceneRenderStateRenderPass::reflectorIndex(renderState, ReflectionUtil::currentEntry());

                        PhysicsComponent* const pComp = nodePtr->get<PhysicsComponent>();
                        Attorney::RenderingCompRenderPass::updateReflection(*rComp, 
                                                                            ReflectionUtil::currentEntry(),
                                                                            pComp->getPosition(),
                                                                            renderState);
                        ReflectionUtil::updateBudget();
                    } else {
                        Attorney::RenderingCompRenderPass::clearReflection(*rComp);
                    }
                }
                
                // While in budget, update refractions
                RefractionUtil::resetBudget();
                for (const RenderPassCuller::VisibleNode& node : nodeCache) {
                    SceneGraphNode_cptr nodePtr = node.second.lock();
                    RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                    if (RefractionUtil::isInBudget()) {
                        PhysicsComponent* const pComp = nodePtr->get<PhysicsComponent>();
                        Attorney::RenderingCompRenderPass::updateRefraction(*rComp,
                                                                             RefractionUtil::currentEntry(),
                                                                             pComp->getPosition(),
                                                                             renderState);
                        RefractionUtil::updateBudget();
                    } else {
                        Attorney::RenderingCompRenderPass::clearRefraction(*rComp);
                    }
                }

            } break;
        };

        postRender(renderState, idx);
        idx++;
    }
}

bool RenderPass::preRender(SceneRenderState& renderState, U32 pass) {
    GFXDevice& GFX = GFX_DEVICE;
    GFX.setRenderStage(_stageFlags[pass]);

    renderState.getCameraMgr().getActiveCamera().renderLookAt();

    bool bindShadowMaps = false;
    switch (_stageFlags[pass]) {
        case RenderStage::DISPLAY: {
            RenderQueue& renderQueue = RenderPassManager::instance().getQueue();
            _lastTotalBinSize = renderQueue.getRenderQueueStackSize();
            bindShadowMaps = true;
            if (Config::USE_HIZ_CULLING) {
                GFX.occlusionCull(RenderPassManager::instance().getBufferData(RenderStage::DISPLAY, 0, 0));
            }
            if (_useZPrePass) {
                GFX.toggleDepthWrites(false);
            }
            GFX.getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._target->begin(_useZPrePass ? _noDepthClear
                                                                                               : RenderTarget::defaultPolicy());
        } break;
        case RenderStage::REFLECTION: {
            bindShadowMaps = true;
        } break;
        case RenderStage::SHADOW: {
        } break;
        case RenderStage::Z_PRE_PASS: {
            GFX.getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._target->begin(_depthOnly);
        } break;
    };
    
    if (bindShadowMaps) {
        LightPool::bindShadowMaps();
    }

    return true;
}

bool RenderPass::postRender(SceneRenderState& renderState, U32 pass) {
    GFXDevice& GFX = GFX_DEVICE;

    RenderQueue& renderQueue = RenderPassManager::instance().getQueue();
    renderQueue.postRender(renderState, _stageFlags[pass]);

    Attorney::SceneRenderPass::debugDraw(renderState.parentScene(), _stageFlags[pass]);

    switch (_stageFlags[pass]) {
        case RenderStage::DISPLAY:
        case RenderStage::Z_PRE_PASS: {
            GFXDevice::RenderTargetWrapper& renderTarget = GFX.getRenderTarget(GFXDevice::RenderTargetID::SCREEN);
            renderTarget._target->end();

            if (_stageFlags[pass] == RenderStage::Z_PRE_PASS) {
                GFX.constructHIZ();
                Attorney::SceneManagerRenderPass::preRender(SceneManager::instance());
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


// This is very hackish but should hold up fine
std::pair<U32, U32> RenderPass::getRenderPassInfoForStages(const vectorImpl<RenderStage>& stages) const {
    U32 maxPasses = 0;
    U32 maxStages = 0;

    //We only care about the first parameter as it will determine the properties for the rest of the stages
    switch (stages[0]) {
        case RenderStage::REFLECTION: {
            maxPasses = Config::MAX_REFLECTIVE_NODES_IN_VIEW;
            maxStages = 6u; // number of cube faces
        }; break;
        case RenderStage::SHADOW: {
            maxPasses = Config::Lighting::MAX_SHADOW_CASTING_LIGHTS;
            // Either CSM or cube map will have the highest stage count.
            maxStages = std::max(Config::Lighting::MAX_SPLITS_PER_LIGHT, 6u);// number of cube faces
        }; break;
        case RenderStage::Z_PRE_PASS:
        case RenderStage::DISPLAY: {
            maxPasses = 1;
            maxStages = 1;
        }; break;
    };


    return std::make_pair(maxPasses, maxStages);
}

};