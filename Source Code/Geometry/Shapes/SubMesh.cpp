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
          to_uint(flag) | to_const_uint(ObjectFlag::OBJECT_FLAG_NO_VB)),
      _visibleToNetwork(true),
      _render(true),
      _ID(0),
      _parentMesh(nullptr)
{
}

SubMesh::~SubMesh()
{
}

void SubMesh::initialiseDrawCommands(SceneGraphNode& sgn,
                                     RenderStage renderStage,
                                     GenericDrawCommands& drawCommandsInOut) {
    RenderingComponent* const renderable = sgn.get<RenderingComponent>();
    assert(renderable != nullptr);

    GenericDrawCommand cmd(PrimitiveType::TRIANGLES,
        getGeometryVB()->getPartitionOffset(_geometryPartitionID),
        getGeometryVB()->getPartitionIndexCount(_geometryPartitionID));

    cmd.sourceBuffer(_parentMesh->getGeometryVB());

    drawCommandsInOut.push_back(cmd);

    Object3D::initialiseDrawCommands(sgn, renderStage, drawCommandsInOut);
}

void SubMesh::setParentMesh(Mesh* const parentMesh) {
    _parentMesh = parentMesh;
    setGeometryVB(_parentMesh->getGeometryVB());
}

};