#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"

/// Mesh bounding box is built from all the SubMesh bounding boxes
bool Mesh::computeBoundingBox(SceneGraphNode* const sgn){
    BoundingBox& bb = sgn->getBoundingBox();

    bb.reset();
    for_each(childrenNodes::value_type& s, sgn->getChildren()){
        bb.Add(s.second->getInitialBoundingBox());
    }
    _maxBoundingBox.Add(bb);
    _maxBoundingBox.setComputed(true);
    return SceneNode::computeBoundingBox(sgn);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode* const sgn){
    for_each(std::string& it, _subMeshes){
        ResourceDescriptor subMesh(it);
        // Find the SubMesh resource
        SubMesh* s = FindResource<SubMesh>(it);
        if(!s) continue;
        REGISTER_TRACKED_DEPENDENCY(s);

        // Add the SubMesh resource as a child
        sgn->addNode(s,sgn->getName()+"_"+it);
        //Hold a reference to the submesh by ID (used for animations)
        _subMeshRefMap.insert(std::make_pair(s->getId(), s));
        s->setParentMesh(this);
    }
    _maxBoundingBox.reset();
    Object3D::postLoad(sgn);
}

void Mesh::onDraw(const RenderStage& currentStage){
    if(getState() != RES_LOADED) return;
    Object3D::onDraw(currentStage);
}

/// Called from SceneGraph "sceneUpdate"
void Mesh::sceneUpdate(const U64 deltaTime,SceneGraphNode* const sgn, SceneState& sceneState){
    Object3D::sceneUpdate(deltaTime, sgn, sceneState);
}