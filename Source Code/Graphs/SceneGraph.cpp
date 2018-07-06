#include "Headers/SceneGraph.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

SceneGraph::SceneGraph(){
    _root = New SceneGraphNode(New SceneRoot);
    _root->setBBExclusionMask(TYPE_SKY | TYPE_LIGHT | TYPE_TRIGGER |TYPE_PARTICLE_EMITTER|TYPE_VEGETATION_GRASS|TYPE_VEGETATION_TREES);
    _root->setSceneGraph(this);
    _updateRunning = false;
}

void SceneGraph::update(){
    _root->checkBoundingBoxes();
    _root->updateTransforms();
}

void SceneGraph::idle(){
    typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;
    for_each(SceneGraphNode* it, _pendingDeletionNodes){
        it->unload();
        it->getParent()->removeNode(it);
        SAFE_DELETE(it);
    }
}

void SceneGraph::print(){
    PRINT_FN(Locale::get("SCENEGRAPH_TITLE"));
    Console::getInstance().toggleTimeStamps(false);
    boost::unique_lock< boost::mutex > lock_access_here(_rootAccessMutex);
    _root->print();
    Console::getInstance().toggleTimeStamps(true);
}

void SceneGraph::startUpdateThread(){
}

void SceneGraph::sceneUpdate(const U64 deltaTime, SceneState& sceneState){
    _root->sceneUpdate(deltaTime, sceneState);
}

void SceneGraph::Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits){
    _root->Intersect(ray,start,end,selectionHits); 
}