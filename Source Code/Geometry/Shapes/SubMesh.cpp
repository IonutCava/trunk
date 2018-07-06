#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Dynamics/Entities/Headers/Impostor.h"

namespace Divide {

SubMesh::SubMesh(GFXDevice& context, ResourceCache& parentCache, const stringImpl& name, ObjectFlag flag)
    : Object3D(context,
               parentCache,
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
                                     const RenderStagePass& renderStagePass,
                                     GenericDrawCommands& drawCommandsInOut) {

    GenericDrawCommand cmd(PrimitiveType::TRIANGLES,
        getGeometryVB()->getPartitionOffset(_geometryPartitionID),
        getGeometryVB()->getPartitionIndexCount(_geometryPartitionID));

    cmd.sourceBuffer(_parentMesh->getGeometryVB());

    drawCommandsInOut.push_back(cmd);

    Object3D::initialiseDrawCommands(sgn, renderStagePass, drawCommandsInOut);
}

void SubMesh::setParentMesh(Mesh* const parentMesh) {
    _parentMesh = parentMesh;
    setGeometryVB(_parentMesh->getGeometryVB());
}

};