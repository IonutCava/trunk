#include "stdafx.h"

#include "Headers/GraphicsResource.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GraphicsResource::GraphicsResource(GFXDevice& context, Type type, I64 GUID, U64 nameHash)
    : _context(context), _type(type), _GUID(GUID), _nameHash(nameHash)
{
    Attorney::GFXDeviceGraphicsResource::onResourceCreate(_context, _type, _GUID, _nameHash);
}

GraphicsResource::~GraphicsResource()
{
    Attorney::GFXDeviceGraphicsResource::onResourceDestroy(_context, _type, _GUID, _nameHash);
}
}; //namespace Divide