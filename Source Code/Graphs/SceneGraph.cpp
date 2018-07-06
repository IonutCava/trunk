#include "stdafx.h"

#include "Headers/SceneGraph.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Geometry/Material/Headers/Material.h"
#include "ECS/Systems/Headers/ECSManager.h"

namespace Divide {

namespace {
    U32 ignoredNodeType = to_base(SceneNodeType::TYPE_ROOT) |
                          to_base(SceneNodeType::TYPE_LIGHT) |
                          to_base(SceneNodeType::TYPE_PARTICLE_EMITTER) |
                          to_base(SceneNodeType::TYPE_TRIGGER) |
                          to_base(SceneNodeType::TYPE_SKY) |
                          to_base(SceneNodeType::TYPE_VEGETATION_GRASS);
};

SceneGraph::SceneGraph(Scene& parentScene)
    : FrameListener(),
      SceneComponent(parentScene),
     _loadComplete(false),
     _octreeChanged(false),
     _ecsEngine(new ECS::ECSEngine()),
     _rootNode(new SceneRoot(parentScene.resourceCache(), 1234))
{
    ECSManager::init(GetECSEngine());

    static const U32 rootMask = to_base(ComponentType::TRANSFORM) |
                                to_base(ComponentType::BOUNDS);

    REGISTER_FRAME_LISTENER(this, 1);
    _root = createSceneGraphNode(*this, PhysicsGroup::GROUP_IGNORE, _rootNode, "ROOT", rootMask);
    _root->get<BoundsComponent>()->lockBBTransforms(true);
    Attorney::SceneNodeSceneGraph::postLoad(*_rootNode, *_root);
    onNodeAdd(*_root);

    U32 octreeNodeMask = to_base(SceneNodeType::TYPE_ROOT) |
                         to_base(SceneNodeType::TYPE_LIGHT) |
                         to_base(SceneNodeType::TYPE_SKY) |
                         to_base(SceneNodeType::TYPE_VEGETATION_GRASS);

    _octree.reset(new Octree(octreeNodeMask));
    _octreeUpdating = false;
}

SceneGraph::~SceneGraph()
{ 
    _octree.reset();
    _allNodes.clear();
    UNREGISTER_FRAME_LISTENER(this);
    Console::d_printfn(Locale::get(_ID("DELETE_SCENEGRAPH")));
    // Should recursively delete the entire scene graph
    unload();
    ECSManager::destroy(GetECSEngine());
}

void SceneGraph::unload()
{
    idle();

    destroySceneGraphNode(_root);
    assert(_root == nullptr);
}


bool SceneGraph::frameStarted(const FrameEvent& evt) {
    _root->processDeleteQueue();

    return true;
}

bool SceneGraph::frameEnded(const FrameEvent& evt) {
    return true;
}

void SceneGraph::unregisterNode(I64 guid, SceneGraphNode::UsageContext usage) {
}

void SceneGraph::onNodeDestroy(SceneGraphNode& oldNode) {
    I64 guid = oldNode.getGUID();

    vectorImpl<SceneGraphNode*>& nodesByType = _nodesByType[to_base(oldNode.getNode()->getType())];

    nodesByType.erase(std::remove_if(std::begin(nodesByType), std::end(nodesByType), 
                                     [guid](SceneGraphNode* node)-> bool
                                     {
                                         return node && node->getGUID() == guid;
                                     }),
                      std::end(nodesByType));

    if (_loadComplete) {
        unregisterNode(guid, oldNode.usageContext());
    }

    Attorney::SceneGraph::onNodeDestroy(_parentScene, oldNode);

    _allNodes.erase(std::remove_if(std::begin(_allNodes), std::end(_allNodes),
                                   [guid](SceneGraphNode* node)-> bool 
                                   {
                                       return node && node->getGUID() == guid;
                                   }),
                    std::end(_allNodes));
}

void SceneGraph::onNodeAdd(SceneGraphNode& newNode) {
    SceneGraphNode* newNodePtr = &newNode;

    _allNodes.push_back(newNodePtr);
    _nodesByType[to_base(newNodePtr->getNode()->getType())].push_back(newNodePtr);
    
    if (_loadComplete) {
        WAIT_FOR_CONDITION(!_octreeUpdating);
        _octreeChanged = _octree->addNode(newNodePtr);
    }
}

void SceneGraph::onNodeTransform(SceneGraphNode& node) {
    if (_loadComplete) {
        node.setUpdateFlag(SceneGraphNode::UpdateFlag::SPATIAL_PARTITION);
    }
}

void SceneGraph::idle()
{
}

bool SceneGraph::removeNodesByType(SceneNodeType nodeType) {
    return _root != nullptr && getRoot().removeNodesByType(nodeType);
}

bool SceneGraph::removeNode(I64 guid) {
    return removeNode(findNode(guid));
}

bool SceneGraph::removeNode(SceneGraphNode* node) {
    if (node) {
        SceneGraphNode* parent = node->getParent();
        if (parent) {
            if (!parent->removeNode(*node)) {
                return false;
            }
        }

        return true;
    }

    return false;
}

void SceneGraph::sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState) {

    GetECSEngine().Update(Time::MicrosecondsToMilliseconds<F32>(deltaTimeUS));

    // Gather all nodes in order
    _orderedNodeList.resize(0);
    _root->getOrderedNodeList(_orderedNodeList);
    for (SceneGraphNode* node : _orderedNodeList) {
        node->sceneUpdate(deltaTimeUS, sceneState);
    }

    if (_loadComplete && false) {
        CreateTask(parentScene().platformContext(),
            [this, deltaTimeUS](const Task& parentTask) mutable
            {
                _octreeUpdating = true;
                if (_octreeChanged) {
                    _octree->updateTree();
                }
                _octree->update(deltaTimeUS);
            },
            [this]() mutable
            {
                _octreeUpdating = false;
            })._task->startTask(Task::TaskPriority::REALTIME_WITH_CALLBACK);
    }
}

void SceneGraph::onCameraUpdate(const Camera& camera) {
    static const U32 s_nodesPerThread = 64;

    I64 guid = camera.getGUID();
    const vec3<F32>& eye = camera.getEye(); 
    const mat4<F32>& world = camera.getWorldMatrix();

    U32 childCount = _root->getChildCount();
    auto updateCamera = [this, &guid, &eye, &world](const Task& parentTask, U32 start, U32 end) {
        auto perChildCull = [this, &guid, &eye, &world](SceneGraphNode& child, I32 i) {
            child.onCameraUpdate(guid, eye, world);
        };

        _root->forEachChild(perChildCull, start, end);
    };

    parallel_for(parentScene().platformContext(), updateCamera, childCount, s_nodesPerThread, Task::TaskPriority::MAX);
}

void SceneGraph::onCameraChange(const Camera& camera) {
    _root->onCameraChange(camera);
}

void SceneGraph::onNetworkSend(U32 frameCount) {
    _root->onNetworkSend(frameCount);
}

void SceneGraph::intersect(const Ray& ray, F32 start, F32 end, vectorImpl<I64>& selectionHits) const {
    _root->intersect(ray, start, end, selectionHits);

    /*if (_loadComplete) {
        WAIT_FOR_CONDITION(!_octreeUpdating);
        U32 filter = to_base(SceneNodeType::TYPE_OBJECT3D);
        SceneGraphNode* collision = _octree->nearestIntersection(ray, start, end, filter)._intersectedObject1.lock();
        if (collision) {
            Console::d_printfn("Octree ray cast [ %s ]", collision->getName().c_str());
        }
    }*/
}

void SceneGraph::postLoad() {
    _octree->addNodes(_allNodes);
    _octreeChanged = true;
    _loadComplete = true;
}


const vectorImpl<SceneGraphNode*>& SceneGraph::getNodesByType(SceneNodeType type) const {
    return _nodesByType[to_base(type)];
}

ECS::EntityManager* SceneGraph::GetEntityManager() {
    return GetECSEngine().GetEntityManager();
}

ECS::EntityManager* SceneGraph::GetEntityManager() const {
    return GetECSEngine().GetEntityManager();
}

ECS::ComponentManager* SceneGraph::GetComponentManager() {
    return GetECSEngine().GetComponentManager();
}

ECS::ComponentManager* SceneGraph::GetComponentManager() const {
    return GetECSEngine().GetComponentManager();
}
};