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

void SubMesh::setParentMesh(Mesh* const parentMesh) { 
	_parentMesh = parentMesh; 
	setGeometryVB(_parentMesh->getGeometryVB());
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
    // If the mesh has animation data, use dynamic VB's if we use software skinning
	_drawCmd.firstIndex(getGeometryVB()->getPartitionOffset(_geometryPartitionId));
	_drawCmd.indexCount(getGeometryVB()->getPartitionCount(_geometryPartitionId));
    Object3D::postLoad(sgn);
}

void SubMesh::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    assert(_parentMesh != nullptr);
    _drawCmd.renderWireframe(sgn->renderWireframe());
    _drawCmd.LoD(getCurrentLOD());
    _drawCmd.drawID(GFX_DEVICE.getDrawID(sgn->getGUID()));
    _drawCmd.stateHash(getDrawStateHash(currentRenderStage));
    _drawCmd.shaderProgram(getDrawShader(currentRenderStage));
    _parentMesh->addDrawCommand(_drawCmd);

    GFX_DEVICE.submitRenderCommand(_parentMesh->getGeometryVB(), _drawCmd);
}