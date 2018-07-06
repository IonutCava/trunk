#include "stdafx.h"

#include "Headers/VertexDataInterface.h"

namespace Divide {

VertexDataInterface::VertexDataInterface(GFXDevice& context)
  : GUIDWrapper(),
    GraphicsResource(context, getGUID())
{
}

VertexDataInterface::~VertexDataInterface()
{
}

}; //namespace Divide
