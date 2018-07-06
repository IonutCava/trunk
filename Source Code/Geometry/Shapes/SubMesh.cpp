#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"

SubMesh::SubMesh(const std::string& name, ObjectFlag flag) : Object3D(name, SUBMESH, flag | OBJECT_FLAG_NO_VB),
                                                             _visibleToNetwork(true),
                                                             _render(true),
                                                             _id(0),
                                                             _parentMesh(nullptr),
                                                             _parentMeshSGN(nullptr)
{
    _drawCmd = GenericDrawCommand(TRIANGLES, 0, 1);
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
    VertexBuffer* vb = _parentMesh->getGeometryVB();
    _drawCmd._cmd.firstIndex =  vb->getPartitionOffset(_geometryPartitionId);
    _drawCmd._cmd.count =       vb->getPartitionCount(_geometryPartitionId);
    Object3D::postLoad(sgn);
}

void SubMesh::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    assert(_parentMesh != nullptr);

    _drawCmd.setLoD(getCurrentLOD());
    _drawCmd.setDrawIDs(GFX_DEVICE.getDrawIDs(sgn->getGUID()));
    _drawCmd.setStateHash(getDrawStateHash(currentRenderStage));
    _drawCmd.setShaderProgram(getDrawShader(currentRenderStage));
    _parentMesh->addDrawCommand(_drawCmd);

    GFX_DEVICE.submitRenderCommand(_parentMesh->getGeometryVB(), _drawCmd);
}