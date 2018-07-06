#include "Headers/RenderBinMesh.h"

RenderBinMesh::RenderBinMesh(const RenderBinType& rbType,const RenderingOrder::List& renderOrder, D32 drawKey) : RenderBin(rbType, renderOrder,drawKey)
{
}

RenderBinMesh::~RenderBinMesh()
{
}

