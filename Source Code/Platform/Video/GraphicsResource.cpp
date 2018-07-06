#include "stdafx.h"

#include "Headers/GraphicsResource.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GraphicsResource::GraphicsResource(GFXDevice& context, Type type, I64 GUID)
    : _context(context), _type(type), _GUID(GUID)
{
    Attorney::GFXDeviceGraphicsResource::onResourceCreate(_context, _type, _GUID);
}

GraphicsResource::~GraphicsResource()
{
    Attorney::GFXDeviceGraphicsResource::onResourceDestroy(_context, _type, _GUID);
}
}; //namespace Divide