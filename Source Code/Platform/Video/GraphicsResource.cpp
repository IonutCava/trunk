#include "Headers/GraphicsResource.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GraphicsResource::GraphicsResource(GFXDevice& context) : _context(context)
{
    Attorney::GFXDeviceGraphicsResource::onResourceCreate(_context);
}

GraphicsResource::~GraphicsResource()
{
    Attorney::GFXDeviceGraphicsResource::onResourceDestroy(_context);
}
}; //namespace Divide