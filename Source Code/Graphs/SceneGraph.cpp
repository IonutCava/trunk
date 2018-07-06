#include "Headers/SceneGraph.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Geometry/Material/Headers/Material.h"

SceneGraph::SceneGraph(){
    SceneNode* root = New SceneRoot();
    _root = New SceneGraphNode(this, root);
    _root->setCastsShadows(false);
    _root->setReceivesShadows(false);
    root->postLoad(_root);
    _root->setBBExclusionMask(TYPE_SKY | TYPE_LIGHT | TYPE_TRIGGER |TYPE_PARTICLE_EMITTER|TYPE_VEGETATION_GRASS|TYPE_VEGETATION_TREES);
    _updateRunning = false;
}

void SceneGraph::idle(){
    for(SceneGraphNode*& it : _pendingDeletionNodes){
        it->unload();
        it->getParent()->removeNode(it);
        SAFE_DELETE(it);
    }
}

void SceneGraph::sceneUpdate(const U64 deltaTime, SceneState& sceneState){
    _root->sceneUpdate(deltaTime, sceneState);
}

void SceneGraph::Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits){
    _root->Intersect(ray,start,end,selectionHits); 
}

void SceneGraph::print(){
    PRINT_FN(Locale::get("SCENEGRAPH_TITLE"));
    Console::getInstance().toggleTimeStamps(false);
    boost::unique_lock< boost::mutex > lock_access_here(_rootAccessMutex);
    printInternal(_root);
    Console::getInstance().toggleTimeStamps(true);
}

///Prints out the SceneGraph structure to the Console
void SceneGraph::printInternal(SceneGraphNode* const sgn){
    if (!sgn)
        return;

    //Starting from the current node
    SceneGraphNode* parent = sgn;
    SceneGraphNode* tempParent = parent;
    U8 i = 0;
    //Count how deep in the graph we are
    //by counting how many ancestors we have before the "root" node
    while (tempParent != nullptr){
        tempParent = tempParent->getParent();
        i++;
    }
    //get out material's name
    Material* mat = parent->getNode()->getMaterial();

    //Some strings to hold the names of our material and shader
    std::string material("none"), shader("none"), depthShader("none");
    //If we have a material
    if (mat){
        //Get the material's name
        material = mat->getName();
        //If we have a shader
        if (mat->getShaderProgram()){
            //Get the shader's name
            shader = mat->getShaderProgram()->getName();
        }
        if (mat->getShaderProgram(SHADOW_STAGE)){
            //Get the depth shader's name
            depthShader = mat->getShaderProgram(SHADOW_STAGE)->getName();
        }
        if (mat->getShaderProgram(Z_PRE_PASS_STAGE)){
            //Get the depth shader's name
            depthShader = mat->getShaderProgram(Z_PRE_PASS_STAGE)->getName();
        }
    }
    //Print our current node's information
    PRINT_FN(Locale::get("PRINT_SCENEGRAPH_NODE"), parent->getName().c_str(),
        parent->getNode()->getName().c_str(),
        material.c_str(),
        shader.c_str(),
        depthShader.c_str());
    //Repeat for each child, but prefix it with the appropriate number of dashes
    //Based on our ancestor counting earlier
    FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, parent->getChildren()){
        for (U8 j = 0; j < i; j++){
            PRINT_F("-");
        }
        printInternal(it.second);
    }
}