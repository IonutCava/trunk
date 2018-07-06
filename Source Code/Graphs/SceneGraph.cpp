#include "Headers/SceneGraph.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {
SceneGraph::SceneGraph() : _root(nullptr)
{
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
}

SceneGraph::~SceneGraph()
{ 
    Console::d_printfn(Locale::get("DELETE_SCENEGRAPH"));
    // Should recursively delete the entire scene graph
    assert(_root.unique());
    _root.reset();
}

void SceneGraph::idle()
{
    if (!_pendingDeletionNodes.empty()) {
        for (std::weak_ptr<SceneGraphNode> node : _pendingDeletionNodes) {
            deleteNode(node, true);
        }
      
        _pendingDeletionNodes.clear();
    }
}

void SceneGraph::deleteNode(std::weak_ptr<SceneGraphNode> node, bool deleteOnAdd) {
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

void SceneGraph::intersect(const Ray& ray, F32 start, F32 end,
                           vectorImpl<std::weak_ptr<SceneGraphNode>>& selectionHits) {
    _root->intersect(ray, start, end, selectionHits);
}


void SceneGraph::onNodeDestroy(SceneGraphNode& oldNode) {
    Attorney::SceneGraph::onNodeDestroy(GET_ACTIVE_SCENE(), oldNode);
}

};