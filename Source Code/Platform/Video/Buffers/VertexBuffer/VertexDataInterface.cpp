#include "Headers/VertexDataInterface.h"

namespace Divide {

VertexDataInterface::VertexDataInterface(GFXDevice& context)
  : GraphicsResource(context),
    GUIDWrapper()
{
}

VertexDataInterface::~VertexDataInterface()
{
}

}; //namespace Divide
