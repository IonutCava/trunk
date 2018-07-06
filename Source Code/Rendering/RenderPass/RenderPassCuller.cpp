#include "Headers/RenderPassCuller.h"

#include "Core/Headers/Kernel.h"
#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace {
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

namespace Divide {

RenderPassCuller::RenderPassCuller() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.reserve(Config::MAX_VISIBLE_NODES);
    }
}

RenderPassCuller::~RenderPassCuller() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.clear();
    }
}

U32 RenderPassCuller::stageToCacheIndex(RenderStage stage) const {
    switch (stage) {
        case RenderStage::REFLECTION:  return 1;
        case RenderStage::SHADOW:  return 2;
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
    nodeCache.resize(0);
    Kernel& kernel = Application::getInstance().getKernel();
    const SceneRenderState& renderState = sceneState.renderState();
    if (renderState.drawGeometry()) {
        _cullingFunction = cullingFunction;
        // No point in updating visual information if the scene disabled object
        // rendering or rendering of their bounding boxes
        SceneGraphNode& root = sceneGraph.getRoot();
        U32 childCount = root.getChildCount();
        _perThreadNodeList.resize(childCount);
        const Camera& camera = renderState.getCameraConst();
        F32 cullMaxDistance = sceneState.generalVisibility();
        TaskHandle cullTask = kernel.AddTask(DELEGATE_CBK_PARAM<bool>());
        for (U32 i = 0; i < childCount; ++i) {
            const SceneGraphNode& child = root.getChild(i, childCount);
            cullTask.addChildTask(kernel.AddTask(DELEGATE_BIND(&RenderPassCuller::frustumCullNode,
                                                               this,
                                                               std::placeholders::_1,
                                                               std::cref(child),
                                                               std::cref(camera),
                                                               stage,
                                                               cullMaxDistance,
                                                               i,
                                                               true)
                                                 )._task);
        }
        cullTask.startTask(Task::TaskPriority::MAX);
        cullTask.wait();

        for (VisibleNodeList& nodeList : _perThreadNodeList) {
            for (VisibleNode& ptr : nodeList) {
                nodeCache.push_back(ptr);
            }
        }
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
    VisibleNodeList& nodes = _perThreadNodeList[nodeListIndex];
    if (clearList) {
        nodes.resize(0);
    }

    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
    bool isVisible = !(currentStage == RenderStage::SHADOW &&
                       !currentNode.get<RenderingComponent>()->castsShadows());

    isVisible = isVisible &&
                currentNode.isActive() &&
                !_cullingFunction(currentNode) &&
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
        
            if (child->isActive() && !_cullingFunction(*child)) {
                vectorAlg::emplace_back(nodes, 0, child);
                addAllChildren(*child, currentStage, nodes);
            }
        }
    }
}

};