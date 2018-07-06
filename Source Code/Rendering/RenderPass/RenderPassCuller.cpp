#include "config.h"

#include "Headers/RenderPassCuller.h"

#include "Core/Headers/TaskPool.h"
#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

namespace {
    static const U32 g_nodesPerCullingPartition = Config::MAX_VISIBLE_NODES / 4u;

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
            SceneGraphNode_cptr nodePtr = node.second.lock();
            return (nodePtr != nullptr && nodePtr->getGUID() == GUID);
        });

    return it != std::cend(cache);
}

void RenderPassCuller::frustumCull(SceneGraph& sceneGraph,
                                   const SceneState& sceneState,
                                   RenderStage stage,
                                   bool async,
                                   const CullingFunction& cullingFunction)
{
    VisibleNodeList& nodeCache = getNodeCache(stage);
    vectorImpl<VisibleNodeList>& nodeList = _perThreadNodeList[to_uint(stage)];
    nodeCache.resize(0);
    const SceneRenderState& renderState = sceneState.renderState();
    if (renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)) {
        _cullingFunction[to_uint(stage)] = cullingFunction;
        // No point in updating visual information if the scene disabled object
        // rendering or rendering of their bounding boxes
        SceneGraphNode& root = sceneGraph.getRoot();
        U32 childCount = root.getChildCount();
        nodeList.resize(childCount);
        const Camera& camera = renderState.getCameraConst();
        F32 cullMaxDistance = sceneState.generalVisibility();
        parallel_for(DELEGATE_BIND(&RenderPassCuller::frumstumPartitionCuller,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3,
                                   std::ref(root),
                                   std::cref(camera),
                                   stage,
                                   cullMaxDistance),
                     childCount,
                     g_nodesPerCullingPartition,
                     Task::TaskPriority::MAX);

        for (const VisibleNodeList& nodeListEntry : nodeList) {
            nodeCache.insert(std::end(nodeCache), std::cbegin(nodeListEntry), std::cend(nodeListEntry));
        }
    }
}

void RenderPassCuller::frumstumPartitionCuller(const std::atomic_bool& stopRequested,
                                               U32 start,
                                               U32 end,
                                               SceneGraphNode& root,
                                               const Camera& camera,
                                               RenderStage stage,
                                               F32 cullMaxDistance)
{
    U32 childCount = 0;
    for (U32 i = start; i < end; ++i) {
        frustumCullNode(stopRequested, root.getChild(i, childCount), camera, stage, cullMaxDistance, i, true);
    }
}
/// This method performs the visibility check on the given node and all of its
/// children and adds them to the RenderQueue
void RenderPassCuller::frustumCullNode(const std::atomic_bool&stopRequested,
                                       const SceneGraphNode& currentNode,
                                       const Camera& currentCamera,
                                       RenderStage currentStage,
                                       F32 cullMaxDistance,
                                       U32 nodeListIndex,
                                       bool clearList)
{
    VisibleNodeList& nodes = _perThreadNodeList[to_uint(currentStage)][nodeListIndex];
    if (clearList) {
        nodes.resize(0);
    }

    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
    bool isVisible = !(currentStage == RenderStage::SHADOW &&
                       !currentNode.get<RenderingComponent>()->castsShadows());

    isVisible = isVisible &&
                currentNode.isActive() &&
                !_cullingFunction[to_uint(currentStage)](currentNode) &&
                !currentNode.cullNode(currentCamera, cullMaxDistance, currentStage, collisionResult);

    if (isVisible && !stopRequested) {
        vectorAlg::emplace_back(nodes, 0, currentNode.shared_from_this());
        if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
            // Parent node intersects the view, so check children
            U32 childCount = currentNode.getChildCount();
            for (U32 i = 0; i < childCount; ++i) {
                frustumCullNode(stopRequested,
                                currentNode.getChild(i, childCount),
                                currentCamera,
                                currentStage,
                                cullMaxDistance,
                                nodeListIndex,
                                false);
            }
        } else {
            // All nodes are in view entirely
            addAllChildren(currentNode, currentStage, nodes);
        }
    }
}

void RenderPassCuller::addAllChildren(const SceneGraphNode& currentNode, RenderStage currentStage, VisibleNodeList& nodes) {
    U32 childCount = currentNode.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        SceneGraphNode_cptr child = currentNode.getChild(i, childCount).shared_from_this();
        if (!(currentStage == RenderStage::SHADOW &&
              !currentNode.get<RenderingComponent>()->castsShadows())) {
        
            if (child->isActive() && !_cullingFunction[to_uint(currentStage)](*child)) {
                vectorAlg::emplace_back(nodes, 0, child);
                addAllChildren(*child, currentStage, nodes);
            }
        }
    }
}

};