#include "stdafx.h"

#include "Headers/VertexDataInterface.h"

namespace Divide {

VDIPool<2048> VertexDataInterface::s_VDIPool;

VertexDataInterface::VertexDataInterface(GFXDevice& context)
  : GraphicsResource(context, GraphicsResource::Type::VERTEX_BUFFER, getGUID(), 0u)
{
    _handle = s_VDIPool.allocate(*this);
}

VertexDataInterface::~VertexDataInterface()
{
    s_VDIPool.deallocate(_handle);
}

}; //namespace Divide
