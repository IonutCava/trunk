#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"

Mesh::Mesh(ObjectFlag flag) : Object3D(MESH,TRIANGLES,flag),
                              _visibleToNetwork(true)
{
    setState(RES_LOADING);
}

Mesh::~Mesh()
{
}

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

void Mesh::addSubMesh(SubMesh* const subMesh){
    //A mesh always has submesh SGN nodes handled separately. No need to track it (will cause double Add/Sub Ref)
    //REGISTER_TRACKED_DEPENDENCY(subMesh);
    _subMeshes.push_back(subMesh->getName());
    //Hold a reference to the submesh by ID (used for animations)
    _subMeshRefMap.insert(std::make_pair(subMesh->getId(), subMesh));
    subMesh->setParentMesh(this);
    _maxBoundingBox.reset();
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode* const sgn){
    for_each(std::string& it, _subMeshes){
        ResourceDescriptor subMesh(it);
        // Find the SubMesh resource
        SubMesh* s = FindResourceImpl<SubMesh>(it);
        // Add the SubMesh resource as a child
        if(s) 
            sgn->addNode(s,sgn->getName()+"_"+it);
    }
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

void Mesh::refModifyCallback(bool increase) {
}