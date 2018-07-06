#include "stdafx.h"

#include "Headers/GraphicsResource.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GraphicsResource::GraphicsResource(GFXDevice& context, I64 GUID) : _context(context), _GUID(GUID)
{
    Attorney::GFXDeviceGraphicsResource::onResourceCreate(_context, _GUID);
}

GraphicsResource::~GraphicsResource()
{
    Attorney::GFXDeviceGraphicsResource::onResourceDestroy(_context, _GUID);
}
}; //namespace Divide