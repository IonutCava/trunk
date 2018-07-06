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
                           _rootNode(MemoryManager_NEW SceneRoot()),
                           _root(*_rootNode, "ROOT")
{
    REGISTER_FRAME_LISTENER(this, 1);

    _root.setBBExclusionMask(
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
    Console::d_printfn(Locale::get(_ID("DELETE_SCENEGRAPH")));
    // Should recursively delete the entire scene graph
    unload();
}

void SceneGraph::unload()
{
    U32 childCount = 0;
    while((childCount = _root.getChildCount()) != 0) {
        _root.removeNode(_root.getChild(childCount - 1, childCount), true);
    }
}

bool SceneGraph::frameStarted(const FrameEvent& evt) {
    return true;
}

bool SceneGraph::frameEnded(const FrameEvent& evt) {
    _root.frameEnded();
    return true;
}

void SceneGraph::unregisterNode(I64 guid, SceneGraphNode::UsageContext usage) {
}

void SceneGraph::onNodeDestroy(SceneGraphNode& oldNode) {
    if (!BitCompare(ignoredNodeType, to_uint(oldNode.getNode<>()->getType()))) {
        I64 guid = oldNode.getGUID();
        unregisterNode(guid, oldNode.usageContext());
    }

    Attorney::SceneGraph::onNodeDestroy(GET_ACTIVE_SCENE(), oldNode);
}

void SceneGraph::onNodeAdd(SceneGraphNode& newNode) {
    if (!BitCompare(ignoredNodeType, to_uint(newNode.getNode<>()->getType()))) {
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
        SceneGraphNode* parent = sgn->getParent();
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
    _root.sceneUpdate(deltaTime, sceneState);
}

void SceneGraph::intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode_wptr>& selectionHits) {
    _root.intersect(ray, start, end, selectionHits);
}

};