#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Dynamics/Entities/Headers/Impostor.h"

namespace Divide {

SubMesh::SubMesh(const stringImpl& name, ObjectFlag flag)
    : Object3D(
          name,
          ObjectType::SUBMESH,
          to_uint(flag) | to_uint(ObjectFlag::OBJECT_FLAG_NO_VB)),
      _visibleToNetwork(true),
      _render(true),
      _ID(0),
      _parentMesh(nullptr)
{
    _drawCmd.primitiveType(PrimitiveType::TRIANGLES);
    _drawCmd.firstIndex(0);
    _drawCmd.indexCount(1);
}

SubMesh::~SubMesh()
{
}

void SubMesh::setParentMesh(Mesh* const parentMesh) {
    _parentMesh = parentMesh;
    setGeometryVB(_parentMesh->getGeometryVB());
    // If the mesh has animation data, use dynamic VB's if we use software
    // skinning
    _drawCmd.firstIndex(
        getGeometryVB()->getPartitionOffset(_geometryPartitionID));
    _drawCmd.indexCount(
        getGeometryVB()->getPartitionCount(_geometryPartitionID));
}

bool SubMesh::computeBoundingBox(SceneGraphNode& sgn) {
    BoundingBox& bb = sgn.getBoundingBox();
    if (bb.isComputed()) {
        return true;
    }

    bb.set(_importBB);
    bb.setComputed(true);

    return SceneNode::computeBoundingBox(sgn);
}

void SubMesh::getDrawCommands(SceneGraphNode& sgn,
                              RenderStage renderStage,
                              SceneRenderState& sceneRenderState,
                              vectorImpl<GenericDrawCommand>& drawCommandsOut) {
    assert(_parentMesh != nullptr);

    RenderingComponent* const renderable =
        sgn.getComponent<RenderingComponent>();
    assert(renderable != nullptr);

    _drawCmd.renderGeometry(renderable->renderGeometry());
    _drawCmd.renderWireframe(renderable->renderWireframe());
    _drawCmd.LoD(renderable->lodLevel());
    _drawCmd.stateHash(renderable->getDrawStateHash(renderStage));
    _drawCmd.shaderProgram(renderable->getDrawShader(renderStage));
    _drawCmd.sourceBuffer(_parentMesh->getGeometryVB());
    //render impostor example
    {
        _drawCmd.sourceBuffer(sgn.getComponent<RenderingComponent>()->getImpostor()->getGeometryVB());
        _drawCmd.indexCount(8);
        _drawCmd.firstIndex(0);
    }
    drawCommandsOut.push_back(_drawCmd);
}
};