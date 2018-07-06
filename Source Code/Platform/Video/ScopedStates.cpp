#include "Headers/ScopedStates.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
namespace GFX {

Scoped2DRendering::Scoped2DRendering(GFXDevice& context)
    : _context(context)
{
    _context.toggle2D(true);
}

Scoped2DRendering::~Scoped2DRendering()
{
    _context.toggle2D(false);
}

ScopedViewport::ScopedViewport(GFXDevice& context, const vec4<I32>& viewport)
    : _context(context)
{
    _context.setViewport(viewport);
}

ScopedViewport::~ScopedViewport()
{
    _context.restoreViewport();
}

ScopedDebugMessage::ScopedDebugMessage(GFXDevice& context, const char* message, I32 id)
    : _context(context)
{
    _context.pushDebugMessage(message, id);
}

ScopedDebugMessage::~ScopedDebugMessage()
{
    _context.popDebugMessage();
}

};  // namespace GFX
};  // namespace Divide