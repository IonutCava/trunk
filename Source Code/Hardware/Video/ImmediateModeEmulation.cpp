#include "Headers/ImmediateModeEmulation.h"
#include "Hardware/Video/Textures/Headers/Texture.h"

IMPrimitive::IMPrimitive() : _inUse(false),
                             _hasLines(false),
                             _canZombify(true),
                             _forceWireframe(false),
                             _zombieCounter(0),
                             _lineWidth(1.0f),
                             _texture(NULL)
{
}

IMPrimitive::~IMPrimitive()
{
    SAFE_DELETE(_texture);
}