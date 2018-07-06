#include "config.h"

#include "Headers/RenderPass.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include "Scenes/Headers/Scene.h"
#include "Geometry/Material/Headers/Material.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
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

RenderPass::BufferData::BufferData(GFXDevice& context)
  : _lastCommandCount(0),
    _lasNodeCount(0)
{
    // This do not need to be persistently mapped as, hopefully, they will only be update once per frame
    // Each pass should have its own set of buffers (shadows, reflection, etc)
    _renderData = context.newSB(1, true, false, BufferUpdateFrequency::OCASSIONAL);
    _renderData->create(to_uint(Config::MAX_VISIBLE_NODES), sizeof(GFXDevice::NodeData));

    _cmdBuffer = context.newSB(1, true, false, BufferUpdateFrequency::OCASSIONAL);
    _cmdBuffer->create(Config::MAX_VISIBLE_NODES, sizeof(IndirectDrawCommand));
    _cmdBuffer->addAtomicCounter(3);
}

RenderPass::BufferData::~BufferData()
{
    _cmdBuffer->destroy();
    _renderData->destroy();
}

RenderPass::BufferDataPool::BufferDataPool(GFXDevice& context, U32 maxBuffers)
{
    // Every visible node will first update this buffer with required data (WorldMatrix, NormalMatrix, Material properties, Bone count, etc)
    // Due to it's potentially huge size, it translates to (as seen by OpenGL) a Shader Storage Buffer that's persistently and coherently mapped
    // We make sure the buffer is large enough to hold data for all of our rendering stages to minimize the number of writes per frame
    // Create a shader buffer to hold all of our indirect draw commands
    // Useful if we need access to the buffer in GLSL/Compute programs
    for (U32 j = 0; j < maxBuffers; ++j) {
        BufferData* data = MemoryManager_NEW BufferData(context);
        _buffers.push_back(data);
    }
}

RenderPass::BufferDataPool::~BufferDataPool()
{
    for (BufferData* buffer : _buffers) {
        MemoryManager::DELETE(buffer);
    }
}

RenderPass::BufferData& RenderPass::BufferDataPool::getBufferData(I32 bufferIndex) {
    assert(bufferIndex >= 0);

    return *_buffers[bufferIndex];
}

RenderPass::RenderPass(RenderPassManager& parent, GFXDevice& context, stringImpl name, U8 sortKey, RenderStage passStageFlag)
    : _parent(parent),
      _context(context),
      _sortKey(sortKey),
      _name(name),
      _stageFlag(passStageFlag)
{
    _lastTotalBinSize = 0;

    U32 count = getBufferCountForStage(_stageFlag);
    _passBuffers = MemoryManager_NEW BufferDataPool(_context, count);

    _drawCommandsCache.reserve(Config::MAX_VISIBLE_NODES);
}

RenderPass::~RenderPass() 
{
    MemoryManager::DELETE(_passBuffers);
}

RenderPass::BufferData&  RenderPass::getBufferData(I32 bufferIndex) {
    return _passBuffers->getBufferData(bufferIndex);
}

void RenderPass::generateDrawCommands() {
    _drawCommandsCache.resize(0);
}

void RenderPass::render(SceneRenderState& renderState) {
    if (_stageFlag != RenderStage::SHADOW) {
        LightPool::bindShadowMaps(_context);
    }

    switch(_stageFlag) {
        case RenderStage::DISPLAY: {
            const RenderTarget& screenRT = _context.renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
            RenderPassManager::PassParams params;
            params.occlusionCull = Config::USE_HIZ_CULLING;
            params.velocityCalc = true;
            params.camera = Attorney::SceneManagerRenderPass::getDefaultCamera(_parent.parent().sceneManager());

            params.stage = _stageFlag;
            params.target = RenderTargetID(RenderTargetUsage::SCREEN);
            params.pass = 0;
            params.doPrePass = Config::USE_Z_PRE_PASS && screenRT.getAttachment(RTAttachment::Type::Depth, 0).used();
            params.occlusionCull = true;
            _parent.doCustomPass(params);
            _lastTotalBinSize = _parent.getQueue().getRenderQueueStackSize();

        } break;
        case RenderStage::SHADOW: {
            Attorney::SceneManagerRenderPass::generateShadowMaps(_parent.parent().sceneManager());
        } break;
        case RenderStage::REFLECTION: {
            /*params.pass = Config::MAX_REFLECTIVE_NODES_IN_VIEW;

            Attorney::SceneRenderStateRenderPass::currentStagePass(renderState, Config::MAX_REFLECTIVE_NODES_IN_VIEW);
            //Part 1 - update envirnoment maps:
            SceneEnvironmentProbePool* envProbPool =  Attorney::SceneRenderPass::getEnvProbes(renderState.parentScene());
            const EnvironmentProbeList& probes = envProbPool->getNearestSorted();
            for (EnvironmentProbe_ptr& probe : probes) {
                probe->refresh();
            }
            RenderPassCuller::VisibleNodeList& mainNodeCache = mgr.getVisibleNodesCache(RenderStage::DISPLAY);
            for (const RenderPassCuller::VisibleNode& node : mainNodeCache) {
                SceneGraphNode_cptr nodePtr = node.second.lock();
                RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                Attorney::RenderingCompRenderPass::updateEnvProbeList(*rComp, probes);
            }

            //Part 2 - update classic reflectors (e.g. mirrors, water, etc)
            //Get list of reflective nodes from the scene manager
            const RenderPassCuller::VisibleNodeList& nodeCache = mgr.getSortedReflectiveNodes();

            // While in budget, update reflections
            ReflectionUtil::resetBudget();
            for (const RenderPassCuller::VisibleNode& node : nodeCache) {
                SceneGraphNode_cptr nodePtr = node.second.lock();
                RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                if (ReflectionUtil::isInBudget()) {
                    Attorney::SceneRenderStateRenderPass::currentStagePass(renderState, ReflectionUtil::currentEntry());

                    PhysicsComponent* const pComp = nodePtr->get<PhysicsComponent>();
                    Attorney::RenderingCompRenderPass::updateReflection(*rComp,
                        ReflectionUtil::currentEntry(),
                        pComp->getPosition(),
                        renderState);
                    ReflectionUtil::updateBudget();
                }
                else {
                    Attorney::RenderingCompRenderPass::clearReflection(*rComp);
                }
            }*/
        } break;
        case RenderStage::REFRACTION: {
            // Get list of refractive nodes from the scene manager
            /*const RenderPassCuller::VisibleNodeList& nodeCache = mgr.getSortedRefractiveNodes();

            // While in budget, update refractions
            RefractionUtil::resetBudget();
            for (const RenderPassCuller::VisibleNode& node : nodeCache) {
                SceneGraphNode_cptr nodePtr = node.second.lock();
                RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                if (RefractionUtil::isInBudget()) {
                    Attorney::SceneRenderStateRenderPass::currentStagePass(renderState, RefractionUtil::currentEntry());

                    PhysicsComponent* const pComp = nodePtr->get<PhysicsComponent>();
                    Attorney::RenderingCompRenderPass::updateRefraction(*rComp,
                        RefractionUtil::currentEntry(),
                        pComp->getPosition(),
                        renderState);
                    RefractionUtil::updateBudget();
                }
                else {
                    Attorney::RenderingCompRenderPass::clearRefraction(*rComp);
                }
            }*/

        } break;
    };
}

// This is very hackish but should hold up fine
U32 RenderPass::getBufferCountForStage(RenderStage stage) const {
    U32 maxPasses = 0;
    U32 maxStages = 0;

    //We only care about the first parameter as it will determine the properties for the rest of the stages
    switch (stage) {
        case RenderStage::REFLECTION: {
            // max reflective nodes and an extra buffer for environment maps
            maxPasses = Config::MAX_REFLECTIVE_NODES_IN_VIEW + 1;
            maxStages = 6u; // number of cube faces
        }; break;
        case RenderStage::REFRACTION: {
            // max reflective nodes and an extra buffer for environment maps
            maxPasses = Config::MAX_REFRACTIVE_NODES_IN_VIEW + 1;
            maxStages = 1u;
        } break;
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


    return maxPasses * maxStages;
}

};