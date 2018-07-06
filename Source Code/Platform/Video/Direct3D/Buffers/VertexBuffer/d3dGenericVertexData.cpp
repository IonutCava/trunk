#include "stdafx.h"

#include "Headers/d3dGenericVertexData.h"

namespace Divide {
IMPLEMENT_CUSTOM_ALLOCATOR(d3dGenericVertexData, 0, 0);

d3dGenericVertexData::d3dGenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name)
    : GenericVertexData(context, ringBufferLength, name)
{
}

d3dGenericVertexData::~d3dGenericVertexData()
{
}

}; //namespace Divide