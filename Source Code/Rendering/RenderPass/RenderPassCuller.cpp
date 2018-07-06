#include "config.h"

#include "Headers/RenderPassCuller.h"

#include "Core/Headers/TaskPool.h"
#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

namespace {
    static const U32 g_nodesPerCullingPartition = Config::MAX_VISIBLE_NODES / 128u;

    template <typename T>
    constexpr T&&
        wrap_lval(typename std::remove_reference<T>::type &&obj) noexcept
    {
        return static_cast<T&&>(obj);
    }

    template <typename T>
    constexpr std::reference_wrapper<typename std::remove_reference<T>::type>
        wrap_lval(typename std::remove_reference<T>::type &obj) noexcept
    {
        return std::ref(obj);
    }
};

RenderPassCuller::RenderPassCuller() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.reserve(Config::MAX_VISIBLE_NODES);
    }
}

RenderPassCuller::~RenderPassCuller() {
    clear();
}

void RenderPassCuller::clear() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.clear();
    }
}

U32 RenderPassCuller::stageToCacheIndex(RenderStage stage) const {
    switch (stage) {
        case RenderStage::REFLECTION:  return 1;
        case RenderStage::REFRACTION: return 2;
        case RenderStage::SHADOW:  return 3;
    };

    return 0;
}

RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) {
    return _visibleNodes[stageToCacheIndex(stage)];
}

const RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) const {
    return _visibleNodes[stageToCacheIndex(stage)];
}

bool RenderPassCuller::wasNodeInView(I64 GUID, RenderStage stage) const {
    const VisibleNodeList& cache = getNodeCache(stage);

    VisibleNodeList::const_iterator it;
    it = std::find_if(std::cbegin(cache), std::cend(cache),
        [GUID](VisibleNode node) {
            const SceneGraphNode* nodePtr = node.second;
            return (nodePtr != nullptr && nodePtr->getGUID() == GUID);
        });

    return it != std::cend(cache);
}

void RenderPassCuller::frustumCull(SceneGraph& sceneGraph,
                                   const SceneState& sceneState,
                                   RenderStage stage,
                                   const CullingFunction& cullingFunction)
{
    const SceneRenderState& renderState = sceneState.renderState();
    if (renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)) {
        _cullingFunction[to_uint(stage)] = cullingFunction;
        const Camera& camera = *Camera::activeCamera();
        F32 cullMaxDistance = renderState.generalVisibility();

        SceneGraphNode& root = sceneGraph.getRoot();
        U32 childCount = root.getChildCount();
        vectorImpl<VisibleNodeList>& nodes = _perThreadNodeList[to_uint(stage)];
        nodes.resize(childCount);

        auto cullIterFunction = [this, &root, &camera, &nodes, stage, cullMaxDistance](const Task& parentTask, U32 start, U32 end) {
            auto perChildCull = [this, &parentTask, &camera, &nodes, start, stage, cullMaxDistance](const SceneGraphNode& child, I32 i) {
                frustumCullNode(parentTask, child, camera, stage, cullMaxDistance, nodes[i], true);
            };
            root.forEachChild(perChildCull, start, end);
        };

        parallel_for(cullIterFunction, childCount, g_nodesPerCullingPartition, Task::TaskPriority::MAX);

        VisibleNodeList& nodeCache = getNodeCache(stage);
        nodeCache.resize(0);
        for (const VisibleNodeList& nodeListEntry : nodes) {
            nodeCache.insert(std::end(nodeCache), std::cbegin(nodeListEntry), std::cend(nodeListEntry));
        }
    }
}

/// This method performs the visibility check on the given node and all of its
/// children and adds them to the RenderQueue
void RenderPassCuller::frustumCullNode(const Task& parentTask,
                                       const SceneGraphNode& currentNode,
                                       const Camera& currentCamera,
                                       RenderStage currentStage,
                                       F32 cullMaxDistance,
                                       VisibleNodeList& nodes,
                                       bool clearList) const
{
    if (clearList) {
        nodes.resize(0);
    }
    if (currentNode.getNode()->getType() == SceneNodeType::TYPE_PARTICLE_EMITTER) {
        int a;
        a = 5;
    }
    // Early out for inactive nodes
    if (!currentNode.isActive()) {
        return;
    }

    // Early out for non-shadow casters during shadow pass
    if (currentStage == RenderStage::SHADOW && !(currentNode.get<RenderingComponent>() &&
                                                 currentNode.get<RenderingComponent>()->castsShadows()))
    {
        return;
    }

    bool isVisible = !_cullingFunction[to_uint(currentStage)](currentNode);

    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
    isVisible = isVisible && !currentNode.cullNode(currentCamera, cullMaxDistance, currentStage, collisionResult);

    if (isVisible && !parentTask.stopRequested()) {
        vectorAlg::emplace_back(nodes, 0, &currentNode);
        if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
            // Parent node intersects the view, so check children
            auto childCull = [this, &parentTask, &currentCamera, &nodes, currentStage, cullMaxDistance](const SceneGraphNode& child) {
                frustumCullNode(parentTask, child, currentCamera, currentStage, cullMaxDistance, nodes, false);
            };

            currentNode.forEachChild(childCull);
        } else {
            // All nodes are in view entirely
            addAllChildren(currentNode, currentStage, nodes);
        }
    }
}

void RenderPassCuller::addAllChildren(const SceneGraphNode& currentNode, RenderStage currentStage, VisibleNodeList& nodes) const {
    currentNode.forEachChild([this, &currentNode, currentStage, &nodes](const SceneGraphNode& child) {
        if (!(currentStage == RenderStage::SHADOW &&   !currentNode.get<RenderingComponent>()->castsShadows())) {
            if (child.isActive() && !_cullingFunction[to_uint(currentStage)](child)) {
                vectorAlg::emplace_back(nodes, 0, &child);
                addAllChildren(child, currentStage, nodes);
            }
        }
    });
}

};