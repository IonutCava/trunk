#include "Headers/SceneGraph.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

SceneGraph::SceneGraph() 
{
    SceneNode* root = New SceneRoot();
    _root = New SceneGraphNode(this, root, "ROOT");
    _root->castsShadows(false);
    _root->receivesShadows(false);
    root->postLoad(_root);
    _root->setBBExclusionMask(TYPE_SKY | TYPE_LIGHT | TYPE_TRIGGER |TYPE_PARTICLE_EMITTER|TYPE_VEGETATION_GRASS|TYPE_VEGETATION_TREES);
}

SceneGraph::~SceneGraph() 
{
    D_PRINT_FN(Locale::get("DELETE_SCENEGRAPH"));
    _root->unload(); //< Should recursively call unload on the entire scene graph
    // Should recursively call delete on the entire scene graph
	SceneNode* root = _root->getNode<SceneRoot>();
    MemoryManager::SAFE_DELETE( _root );
	// Delete the root scene node
    MemoryManager::SAFE_DELETE( root );
}

void SceneGraph::idle() {
    for ( SceneGraphNode*& it : _pendingDeletionNodes ) {
        it->unload();
        it->getParent()->removeNode( it );
        MemoryManager::SAFE_DELETE( it );
    }
    _pendingDeletionNodes.clear();
}

void SceneGraph::sceneUpdate(const U64 deltaTime, SceneState& sceneState){
    _root->sceneUpdate(deltaTime, sceneState);
}

void SceneGraph::Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits){
    _root->Intersect(ray,start,end,selectionHits); 
}

void SceneGraph::print() {
    PRINT_FN(Locale::get("SCENEGRAPH_TITLE"));
    Console::getInstance().toggleTimeStamps(false);
    printInternal(_root);
    Console::getInstance().toggleTimeStamps(true);
}

///Prints out the SceneGraph structure to the Console
void SceneGraph::printInternal(SceneGraphNode* const sgn){
	if ( !sgn ) {
		return;
	}
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
    Material* mat = parent->getMaterialInstance();

    //Some strings to hold the names of our material and shader
    stringImpl material("none"), shader("none"), depthShader("none");
    //If we have a material
    if (mat){
        //Get the material's name
        material = mat->getName();
        //If we have a shader
        if (mat->getShaderInfo().getProgram()){
            //Get the shader's name
            shader = mat->getShaderInfo().getProgram()->getName();
        }
        if (mat->getShaderInfo(SHADOW_STAGE).getProgram()){
            //Get the depth shader's name
            depthShader = mat->getShaderInfo(SHADOW_STAGE).getProgram()->getName();
        }
        if (mat->getShaderInfo(Z_PRE_PASS_STAGE).getProgram()){
            //Get the depth shader's name
            depthShader = mat->getShaderInfo(Z_PRE_PASS_STAGE).getProgram()->getName();
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
	for (SceneGraphNode::NodeChildren::value_type& it : parent->getChildren()) {
        for ( U8 j = 0; j < i; j++ ) {
            PRINT_F( "-" );
        }
        printInternal( it.second );
    }
}

};