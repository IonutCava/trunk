#include "stdafx.h"

#include "config.h"

#include "Headers/RenderPassCuller.h"

#include "Core/Headers/EngineTaskPool.h"
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

RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) {
    return _visibleNodes[to_U32(stage)];
}

const RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) const {
    return _visibleNodes[to_U32(stage)];
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

void RenderPassCuller::frustumCull(PlatformContext& context,
                                   SceneGraph& sceneGraph,
                                   const SceneState& sceneState,
                                   RenderStage stage,
                                   const CullingFunction& cullingFunction,
                                   const Camera& camera)
{
    const SceneRenderState& renderState = sceneState.renderState();
    if (renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)) {
        _cullingFunction[to_U32(stage)] = cullingFunction;

        F32 cullMaxDistance = renderState.generalVisibility();

        SceneGraphNode& root = sceneGraph.getRoot();
        U32 childCount = root.getChildCount();
        vectorImpl<VisibleNodeList>& nodes = _perThreadNodeList[to_U32(stage)];
        nodes.resize(childCount);

        auto cullIterFunction = [this, &root, &camera, &nodes, &stage, cullMaxDistance](const Task& parentTask, U32 start, U32 end) {
            auto perChildCull = [this, &parentTask, &camera, &nodes, start, &stage, cullMaxDistance](const SceneGraphNode& child, I32 i) {
                frustumCullNode(parentTask, child, camera, stage, cullMaxDistance, nodes[i], true);
            };
            root.forEachChild(perChildCull, start, end);
        };

        parallel_for(context,
                     cullIterFunction,
                     childCount,
                     g_nodesPerCullingPartition,
                     Task::TaskPriority::MAX);

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
                                       RenderStage stage,
                                       F32 cullMaxDistance,
                                       VisibleNodeList& nodes,
                                       bool clearList) const
{
    if (clearList) {
        nodes.resize(0);
    }
    // Early out for inactive nodes
    if (!currentNode.isActive()) {
        return;
    }

    // Early out for non-shadow casters during shadow pass
    if (stage == RenderStage::SHADOW && !(currentNode.get<RenderingComponent>() &&
                                          currentNode.get<RenderingComponent>()->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS)))
    {
        return;
    }

    bool isVisible = !_cullingFunction[to_U32(stage)](currentNode);

    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
    isVisible = isVisible && !currentNode.cullNode(currentCamera, cullMaxDistance, stage, collisionResult);

    if (isVisible && !parentTask.stopRequested()) {
        vectorAlg::emplace_back(nodes, 0, &currentNode);
        if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
            // Parent node intersects the view, so check children
            auto childCull = [this, &parentTask, &currentCamera, &nodes, &stage, cullMaxDistance](const SceneGraphNode& child) {
                frustumCullNode(parentTask, child, currentCamera, stage, cullMaxDistance, nodes, false);
            };

            currentNode.forEachChild(childCull);
        } else {
            // All nodes are in view entirely
            addAllChildren(currentNode, stage, nodes);
        }
    }
}

void RenderPassCuller::addAllChildren(const SceneGraphNode& currentNode, RenderStage stage, VisibleNodeList& nodes) const {
    currentNode.forEachChild([this, &currentNode, &stage, &nodes](const SceneGraphNode& child) {
        if (!(stage == RenderStage::SHADOW &&   !currentNode.get<RenderingComponent>()->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS))) {
            if (child.isActive() && !_cullingFunction[to_U32(stage)](child)) {
                vectorAlg::emplace_back(nodes, 0, &child);
                addAllChildren(child, stage, nodes);
            }
        }
    });
}

};