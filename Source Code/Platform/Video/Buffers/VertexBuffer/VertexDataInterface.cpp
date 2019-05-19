#include "stdafx.h"

#include "Headers/VertexDataInterface.h"

namespace Divide {

VertexDataInterface::VertexDataInterface(GFXDevice& context)
  : GraphicsResource(context, GraphicsResource::Type::VERTEX_BUFFER, getGUID(), 0u)
{
}

VertexDataInterface::~VertexDataInterface()
{
}

}; //namespace Divide
