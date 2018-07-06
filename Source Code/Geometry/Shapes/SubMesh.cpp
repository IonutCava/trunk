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
}

SubMesh::~SubMesh()
{
}

void SubMesh::postLoad(SceneGraphNode& sgn) {
    Object3D::postLoad(sgn);

    RenderingComponent* const renderable = sgn.getComponent<RenderingComponent>();
    assert(renderable != nullptr);

    GenericDrawCommand cmd(PrimitiveType::TRIANGLES,
        getGeometryVB()->getPartitionOffset(_geometryPartitionID),
        getGeometryVB()->getPartitionCount(_geometryPartitionID));

    cmd.sourceBuffer(_parentMesh->getGeometryVB());

    for (U32 i = 0; i < to_uint(RenderStage::COUNT); ++i) {
        GFXDevice::RenderPackage& pkg = 
            Attorney::RenderingCompSceneNode::getDrawPackage(*renderable, static_cast<RenderStage>(i));
        pkg._drawCommands.push_back(cmd);
    }
}

void SubMesh::setParentMesh(Mesh* const parentMesh) {
    _parentMesh = parentMesh;
    setGeometryVB(_parentMesh->getGeometryVB());
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

bool SubMesh::getDrawCommands(SceneGraphNode& sgn,
                              RenderStage renderStage,
                              const SceneRenderState& sceneRenderState,
                              vectorImpl<GenericDrawCommand>& drawCommandsOut) {

    RenderingComponent* const renderable = sgn.getComponent<RenderingComponent>();

    GenericDrawCommand& cmd = drawCommandsOut.front();

    cmd.renderGeometry(renderable->renderGeometry());
    cmd.renderWireframe(renderable->renderWireframe());
    cmd.LoD(renderable->lodLevel());
    cmd.stateHash(renderable->getDrawStateHash(renderStage));
    cmd.shaderProgram(renderable->getDrawShader(renderStage));
    
    return Object3D::getDrawCommands(sgn, renderStage, sceneRenderState, drawCommandsOut);
}

};