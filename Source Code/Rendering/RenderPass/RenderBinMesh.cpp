#include "Headers/RenderBinMesh.h"

namespace Divide {

RenderBinMesh::RenderBinMesh(const RenderBinType& rbType,
                             const RenderingOrder::List& renderOrder,
                             D32 drawKey)
    : RenderBin(rbType, renderOrder, drawKey) {}

RenderBinMesh::~RenderBinMesh() {}
};