#include "Headers/SceneGraph.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

namespace {
    U32 ignoredNodeType = to_uint(SceneNodeType::TYPE_ROOT) |
                          to_uint(SceneNodeType::TYPE_LIGHT) |
                          to_uint(SceneNodeType::TYPE_PARTICLE_EMITTER) |
                          to_uint(SceneNodeType::TYPE_TRIGGER) |
                          to_uint(SceneNodeType::TYPE_SKY) |
                          to_uint(SceneNodeType::TYPE_VEGETATION_GRASS);
};

SceneGraph::SceneGraph() : FrameListener(),
                           _root(nullptr)
{
    REGISTER_FRAME_LISTENER(this, 1);

    SceneNode* rootNode = MemoryManager_NEW SceneRoot();
    _root = std::make_shared<SceneGraphNode>(*rootNode, "ROOT");
    _root->getComponent<RenderingComponent>()->castsShadows(false);
    _root->getComponent<RenderingComponent>()->receivesShadows(false);
    _root->setBBExclusionMask(
        to_uint(SceneNodeType::TYPE_SKY) |
        to_uint(SceneNodeType::TYPE_LIGHT) |
        to_uint(SceneNodeType::TYPE_TRIGGER) |
        to_uint(SceneNodeType::TYPE_PARTICLE_EMITTER) |
        to_uint(SceneNodeType::TYPE_VEGETATION_GRASS) |
        to_uint(SceneNodeType::TYPE_VEGETATION_TREES));
    onNodeAdd(_root);
}

SceneGraph::~SceneGraph()
{ 
    UNREGISTER_FRAME_LISTENER(this);
    Console::d_printfn(Locale::get("DELETE_SCENEGRAPH"));
    // Should recursively delete the entire scene graph
    assert(_root.unique());
    _root.reset();
}

bool SceneGraph::frameStarted(const FrameEvent& evt) {
    return true;
}

bool SceneGraph::frameEnded(const FrameEvent& evt) {
    _root->frameEnded();
    return true;
}

void SceneGraph::unregisterNode(I64 guid, SceneGraphNode::UsageContext usage) {
    /*if (usage == SceneGraphNode::UsageContext::NODE_DYNAMIC) {
        _dynamicNodes.erase(
            std::remove_if(std::begin(_dynamicNodes), std::end(_dynamicNodes),
                [&guid](SceneGraphNode_wptr node) -> bool {
                    return node.lock() && node.lock()->getGUID() == guid;
                }),
            std::end(_dynamicNodes));
    } else {
        _staticNodes.erase(
            std::remove_if(std::begin(_staticNodes), std::end(_staticNodes),
                [&guid](SceneGraphNode_wptr node) -> bool {
                    return node.lock() && node.lock()->getGUID() == guid;
                }),
            std::end(_staticNodes));
    }*/
}

void SceneGraph::onNodeDestroy(SceneGraphNode& oldNode) {
    if (!BitCompare(ignoredNodeType, to_uint(oldNode.getNode<>()->getType()))) {
        I64 guid = oldNode.getGUID();
        unregisterNode(guid, oldNode.usageContext());
    }

    Attorney::SceneGraph::onNodeDestroy(GET_ACTIVE_SCENE(), oldNode);
}

void SceneGraph::onNodeAdd(SceneGraphNode_ptr newNode) {
    if (!BitCompare(ignoredNodeType, to_uint(newNode->getNode<>()->getType()))) {
        /*if (newNode->usageContext() == SceneGraphNode::UsageContext::NODE_DYNAMIC) {
            _dynamicNodes.push_back(newNode);
        } else {
            _staticNodes.push_back(newNode);
        }*/
    }
}

void SceneGraph::idle()
{
    if (!_pendingDeletionNodes.empty()) {
        for (SceneGraphNode_wptr node : _pendingDeletionNodes) {
            deleteNode(node, true);
        }
      
        _pendingDeletionNodes.clear();
    }
}

void SceneGraph::deleteNode(SceneGraphNode_wptr node, bool deleteOnAdd) {
    SceneGraphNode_ptr sgn = node.lock();
    if (!sgn) {
        return;
    }
    if (deleteOnAdd) {
        SceneGraphNode_ptr parent = sgn->getParent().lock();
        if (parent) {
            parent->removeNode(sgn->getName(), false);
        }

        assert(sgn.unique());
        sgn.reset();
    } else {
        _pendingDeletionNodes.push_back(node);
    }
}

void SceneGraph::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    _root->sceneUpdate(deltaTime, sceneState);
}

void SceneGraph::intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode_wptr>& selectionHits) {
    _root->intersect(ray, start, end, selectionHits);
}

};