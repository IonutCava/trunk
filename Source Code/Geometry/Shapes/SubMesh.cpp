#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"

SubMesh::SubMesh(const std::string& name, ObjectFlag flag) : Object3D(name, SUBMESH, PrimitiveType_PLACEHOLDER, flag),
                                                             _visibleToNetwork(true),
                                                             _render(true),
                                                             _id(0),
                                                             _geometryPartitionId(0),
                                                             _parentMesh(nullptr),
                                                             _parentMeshSGN(nullptr)
{
}

SubMesh::~SubMesh()
{
}

bool SubMesh::computeBoundingBox(SceneGraphNode* const sgn){
    BoundingBox& bb = sgn->getBoundingBox();
    if(bb.isComputed())
        return true;

    bb.set(_importBB);
    bb.setComputed(true);

    return SceneNode::computeBoundingBox(sgn);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SubMesh::postLoad(SceneGraphNode* const sgn){
    //sgn->getTransform()->setTransforms(_localMatrix);
    /// If the mesh has animation data, use dynamic VB's if we use software skinning
    _renderInstance->buffer(_parentMesh->getGeometryVB());

    Object3D::postLoad(sgn);
}

void SubMesh::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState){
    assert(_parentMesh != nullptr);
    VertexBuffer* vb = _parentMesh->getGeometryVB();
    VertexBuffer::DeferredDrawCommand drawCmd;
    drawCmd._cmd.count = vb->getPartitionCount(_geometryPartitionId);
    drawCmd._cmd.firstIndex = vb->getPartitionOffset(_geometryPartitionId);
    drawCmd._unsignedData = _geometryPartitionId;

    _parentMesh->addDrawCommand(drawCmd, _drawShader);
    _renderInstance->deferredDrawCommand(drawCmd);

    Object3D::render(sgn, sceneRenderState);
}