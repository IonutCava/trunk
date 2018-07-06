#include "Headers/SceneGraph.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {
SceneGraph::SceneGraph() : _root(nullptr) { load(); }

SceneGraph::~SceneGraph() { unload(); }

void SceneGraph::load() {
    if (_root != nullptr) {
        return;
    }

    SceneNode* rootNode = MemoryManager_NEW SceneRoot();
    _root.reset(MemoryManager_NEW SceneGraphNode(*rootNode, "ROOT"));
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

void SceneGraph::unload() {
    if (_root == nullptr) {
        return;
    }
    Console::d_printfn(Locale::get("DELETE_SCENEGRAPH"));
    SceneNode* root = _root->getNode<SceneRoot>();
    // Delete the root scene node
    MemoryManager::DELETE(root);
    // Should recursively call unload on the entire scene graph
    _root->unload();
    _root.reset(nullptr);
}

void SceneGraph::idle() {
    for (SceneGraphNode*& it : _pendingDeletionNodes) {
        it->unload();
        it->getParent()->removeNode(*it);
    }

    MemoryManager::DELETE_VECTOR(_pendingDeletionNodes);
}

void SceneGraph::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    _root->sceneUpdate(deltaTime, sceneState);
}

void SceneGraph::intersect(const Ray& ray, F32 start, F32 end,
                           vectorImpl<SceneGraphNode*>& selectionHits) {
    _root->intersect(ray, start, end, selectionHits);
}

void SceneGraph::print() {
    Console::printfn(Locale::get("SCENEGRAPH_TITLE"));
    Console::toggleTimeStamps(false);
    printInternal(getRoot());
    Console::toggleTimeStamps(true);
}

/// Prints out the SceneGraph structure to the Console
void SceneGraph::printInternal(SceneGraphNode& sgn) {
    // Starting from the current node
    SceneGraphNode* parent = &sgn;
    SceneGraphNode* tempParent = parent;
    U8 i = 0;
    // Count how deep in the graph we are
    // by counting how many ancestors we have before the "root" node
    while (tempParent != nullptr) {
        tempParent = tempParent->getParent();
        i++;
    }
    // get out material's name
    Material* mat = nullptr;
    RenderingComponent* const renderable =
        parent->getComponent<RenderingComponent>();
    if (renderable) {
        mat = renderable->getMaterialInstance();
    }
    // Some strings to hold the names of our material and shader
    stringImpl material("none"), shader("none"), depthShader("none");
    // If we have a material
    if (mat) {
        // Get the material's name
        material = mat->getName();
        // If we have a shader
        if (mat->getShaderInfo().getProgram()) {
            // Get the shader's name
            shader = mat->getShaderInfo().getProgram()->getName();
        }
        if (mat->getShaderInfo(RenderStage::SHADOW).getProgram()) {
            // Get the depth shader's name
            depthShader = mat->getShaderInfo(RenderStage::SHADOW)
                              .getProgram()
                              ->getName();
        }
        if (mat->getShaderInfo(RenderStage::Z_PRE_PASS).getProgram()) {
            // Get the depth shader's name
            depthShader = mat->getShaderInfo(RenderStage::Z_PRE_PASS)
                              .getProgram()
                              ->getName();
        }
    }
    // Print our current node's information
    Console::printfn(Locale::get("PRINT_SCENEGRAPH_NODE"),
                     parent->getName().c_str(),
                     parent->getNode()->getName().c_str(), material.c_str(),
                     shader.c_str(), depthShader.c_str());
    // Repeat for each child, but prefix it with the appropriate number of
    // dashes
    // Based on our ancestor counting earlier
    for (SceneGraphNode::NodeChildren::value_type& it : parent->getChildren()) {
        for (U8 j = 0; j < i; j++) {
            Console::printf("-");
        }
        printInternal(*it.second);
    }
}
};