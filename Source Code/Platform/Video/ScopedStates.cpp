#include "stdafx.h"

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

ScopedDebugMessage::ScopedDebugMessage(GFXDevice& context, const stringImpl& message, I32 id)
    : _context(context)
{
    if (Config::ENABLE_GPU_VALIDATION) {
        _context.pushDebugMessage(message.c_str(), id);
    }
}

ScopedDebugMessage::~ScopedDebugMessage()
{
    if (Config::ENABLE_GPU_VALIDATION) {
        _context.popDebugMessage();
    }
}

};  // namespace GFX
};  // namespace Divide