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
#include "ECS/Components/Headers/BoundsComponent.h"

namespace Divide {

namespace {
    constexpr U32 g_nodesPerEventPartition = 16u;
    constexpr U32 g_nodesPerUpdatePartition = 32u;
};

SceneGraph::SceneGraph(Scene& parentScene)
    : FrameListener(),
      SceneComponent(parentScene),
     _loadComplete(false),
     _octreeChanged(false),
     _nodeListChanged(false)
{
    _ecsManager = std::make_unique<ECSManager>(parentScene.context(), GetECSEngine());

    REGISTER_FRAME_LISTENER(this, 1);

    SceneGraphNodeDescriptor rootDescriptor = {};
    rootDescriptor._name = "ROOT";
    rootDescriptor._node = std::make_shared<SceneNode>(parentScene.resourceCache(), GUIDWrapper::generateGUID(), "ROOT", "ROOT", "", SceneNodeType::TYPE_ROOT, to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS));
    rootDescriptor._componentMask = to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS);
    rootDescriptor._usageContext = NodeUsageContext::NODE_STATIC;

    _root = createSceneGraphNode(*this, rootDescriptor);
    _root->postLoad();

    //Attorney::SceneNodeSceneGraph::postLoad(*rootSGN.getNode(), rootSGN);
    onNodeAdd(*_root);

    constexpr U16 octreeNodeMask = to_base(SceneNodeType::TYPE_ROOT) |
                                   to_base(SceneNodeType::TYPE_SKY) |
                                   to_base(SceneNodeType::TYPE_INFINITEPLANE) |
                                   to_base(SceneNodeType::TYPE_VEGETATION);

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
}

void SceneGraph::unload()
{
    idle();

    destroySceneGraphNode(_root);
    assert(_root == nullptr);
}

void SceneGraph::addToDeleteQueue(SceneGraphNode* node, size_t childIdx) {
    UniqueLock<SharedMutex> w_lock(_pendingDeletionLock);
    vectorEASTL<size_t>& list = _pendingDeletion[node];
    if (eastl::find(eastl::cbegin(list), eastl::cend(list), childIdx) == eastl::cend(list))
    {
        list.push_back(childIdx);
    }
}

void SceneGraph::onNodeDestroy(SceneGraphNode& oldNode) {
    I64 guid = oldNode.getGUID();

    if (guid == _root->getGUID()) {
        return;
    }

    eastl::erase_if(_nodesByType[to_base(oldNode.getNode().type())],
                    [guid](SceneGraphNode* node)-> bool
                    {
                        return node && node->getGUID() == guid;
                    });

    Attorney::SceneGraph::onNodeDestroy(_parentScene, oldNode);

    eastl::erase_if(_allNodes,
                    [guid](SceneGraphNode* node)-> bool 
                    {
                        return node && node->getGUID() == guid;
                    });

    _nodeListChanged = true;
}

void SceneGraph::onNodeAdd(SceneGraphNode& newNode) {
    SceneGraphNode* newNodePtr = &newNode;

    _allNodes.push_back(newNodePtr);
    _nodesByType[to_base(newNodePtr->getNode().type())].push_back(newNodePtr);

    _nodeListChanged = true;

    if (_loadComplete) {
        WAIT_FOR_CONDITION(!_octreeUpdating);
        _octreeChanged = _octree->addNode(newNodePtr);
    }
}

void SceneGraph::onNodeTransform(SceneGraphNode& node) {
    if (_loadComplete) {
        node.setFlag(SceneGraphNode::Flags::SPATIAL_PARTITION_UPDATE_QUEUED);
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
        SceneGraphNode* parent = node->parent();
        if (parent) {
            if (!parent->removeChildNode(*node, true)) {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool SceneGraph::frameStarted(const FrameEvent& evt) {
    UniqueLock<SharedMutex> lock(_pendingDeletionLock);
    if (!_pendingDeletion.empty()) {
        for (auto entry : _pendingDeletion) {
            if (entry.first != nullptr) {
                Attorney::SceneGraphNodeSceneGraph::processDeleteQueue(*entry.first, entry.second);
            }
        }
        _pendingDeletion.clear();
    }

    // Gather all nodes in order
    if (_nodeListChanged)
    {
        _orderedNodeList.resize(0);
        Attorney::SceneGraphNodeSceneGraph::getOrderedNodeList(*_root, _orderedNodeList);
        _nodeListChanged = false;
    }

    for (SceneGraphNode* node : _orderedNodeList) {
        node->frameStarted();
    }

    return true;
}

bool SceneGraph::frameEnded(const FrameEvent& evt) {
    for (SceneGraphNode* node : _orderedNodeList) {
        node->frameEnded();
    }
 
    return true;
}

void SceneGraph::sceneUpdate(const U64 deltaTimeUS, SceneState& sceneState) {
    OPTICK_EVENT();

    const F32 msTime = Time::MicrosecondsToMilliseconds<F32>(deltaTimeUS);
    {
        OPTICK_EVENT("ECS::PreUpdate");
        GetECSEngine().PreUpdate(msTime);
    }
    {
        OPTICK_EVENT("ECS::Update");
        GetECSEngine().Update(msTime);
    }
    {
        OPTICK_EVENT("ECS::PostUpdate");
        GetECSEngine().PostUpdate(msTime);
    }

    PlatformContext& context = parentScene().context();

    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = to_U32(_orderedNodeList.size());
    descriptor._partitionSize = g_nodesPerEventPartition;

    parallel_for(context,
                    [this](const Task* parentTask, U32 start, U32 end) {
                        for (U32 i = start; i < end; ++i) {
                            SceneGraphNode* node = _orderedNodeList[i];
                            Attorney::SceneGraphNodeSceneGraph::processEvents(*node);
                        }
                    },
                    descriptor);

#if 0 // need a flag maybe, per node, that returns true if the update can be run in parallel. Otherwise, add to a separate list and parse serially after the parallel_for

    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = to_U32(_orderedNodeList.size());
    descriptor._partitionSize = g_nodesPerUpdatePartition;

    parallel_for(context,
                    [this, deltaTimeUS, &sceneState](const Task& parentTask, U32 start, U32 end) {
                        for (U32 i = start; i < end; ++i) {
                            SceneGraphNode* node = _orderedNodeList[i];
                            node->sceneUpdate(deltaTimeUS, sceneState);
                        }
                    },
                    descriptor,
                    "Process Node Events");
#else
    for (SceneGraphNode* node : _orderedNodeList) {
        node->sceneUpdate(deltaTimeUS, sceneState);
    }
#endif

    if (_loadComplete) {
        Start(*CreateTask(context,
            [this, deltaTimeUS](const Task& parentTask) mutable
            {
                OPTICK_EVENT("Octree Update");
                _octreeUpdating = true;
                if (_octreeChanged) {
                    _octree->updateTree();
                }
                _octree->update(deltaTimeUS);
            }), 
            //TaskPriority::DONT_CARE,
            TaskPriority::REALTIME,
            [this]() { _octreeUpdating = false; });
    }
}

void SceneGraph::onStartUpdateLoop(const U8 loopNumber) {
    ACKNOWLEDGE_UNUSED(loopNumber);
}

void SceneGraph::onNetworkSend(U32 frameCount) {
    Attorney::SceneGraphNodeSceneGraph::onNetworkSend(*_root, frameCount);
}

bool SceneGraph::intersect(const Ray& ray, F32 start, F32 end, vectorEASTL<SGNRayResult>& intersections) const {
    return _root->intersect(ray, start, end, intersections);

    /*if (_loadComplete) {
        WAIT_FOR_CONDITION(!_octreeUpdating);
        U32 filter = to_base(SceneNodeType::TYPE_OBJECT3D);
        SceneGraphNode* collision = _octree->nearestIntersection(ray, start, end, filter)._intersectedObject1.lock();
        if (collision) {
            Console::d_printfn("Octree ray cast [ %s ]", collision->name().c_str());
        }
    }*/
}

void SceneGraph::postLoad() {
    _octree->addNodes(_allNodes);
    _octreeChanged = true;
    _loadComplete = true;
}

SceneGraphNode* SceneGraph::createSceneGraphNode(SceneGraph& sceneGraph, const SceneGraphNodeDescriptor& descriptor) {
    UniqueLock<Mutex> u_lock(_nodeCreateMutex);

    ECS::EntityId nodeID = GetEntityManager()->CreateEntity<SceneGraphNode>(sceneGraph, descriptor);
    return static_cast<SceneGraphNode*>(GetEntityManager()->GetEntity(nodeID));
}

void SceneGraph::destroySceneGraphNode(SceneGraphNode*& node, bool inPlace) {
    if (node) {
        if (inPlace) {
            GetEntityManager()->DestroyAndRemoveEntity(node->GetEntityID());
        } else {
            GetEntityManager()->DestroyEntity(node->GetEntityID());
        }
        node = nullptr;
    }
}

const vectorEASTL<SceneGraphNode*>& SceneGraph::getNodesByType(SceneNodeType type) const {
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

bool SceneGraph::saveCache(ByteBuffer& outputBuffer) const {
    return Attorney::SceneGraphNodeSceneGraph::saveCache(*_root, outputBuffer);
}

bool SceneGraph::loadCache(ByteBuffer& inputBuffer) {
    return Attorney::SceneGraphNodeSceneGraph::loadCache(*_root, inputBuffer);
}

namespace {
    boost::property_tree::ptree dumpSGNtoAssets(const SceneGraphNode* node) {
        boost::property_tree::ptree entry;
        entry.put("<xmlattr>.name", node->name().c_str());
        entry.put("<xmlattr>.type", node->getNode().getTypeName());

        node->forEachChild([&entry](const SceneGraphNode* child, I32 /*childIdx*/) {
            if (child->serialize()) {
                entry.add_child("node", dumpSGNtoAssets(child));
            }
        });

        return entry;
    }
};

void SceneGraph::saveToXML(DELEGATE<void, const char*> msgCallback) const {
    const Str256& scenePath = Paths::g_xmlDataLocation + Paths::g_scenesLocation;
    const boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);

    Str256 sceneLocation(scenePath + "/" + parentScene().resourceName());

    {
        boost::property_tree::ptree pt;
        pt.add_child("entities.node", dumpSGNtoAssets(&getRoot()));

        copyFile((sceneLocation + "/").c_str(), "assets.xml", (sceneLocation + "/").c_str(), "assets.xml.bak", true);
        write_xml((sceneLocation + "/" + "assets.xml").c_str(), pt, std::locale(), settings);
    }

    getRoot().forEachChild([&sceneLocation, &msgCallback](const SceneGraphNode* child, I32 /*childIdx*/) {
        child->saveToXML(sceneLocation, msgCallback);
    });
}

void SceneGraph::loadFromXML() {

}
};