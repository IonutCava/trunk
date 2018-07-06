#include "Headers/d3dGenericVertexData.h"

namespace Divide {
IMPLEMENT_CUSTOM_ALLOCATOR(d3dGenericVertexData, 0, 0);

d3dGenericVertexData::d3dGenericVertexData(GFXDevice& context, const U32 ringBufferLength)
    : GenericVertexData(context, ringBufferLength)
{
}

d3dGenericVertexData::~d3dGenericVertexData()
{
}

}; //namespace Divide