#include "Headers/ScopedStates.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
namespace GFX {

Scoped2DRendering::Scoped2DRendering() {
    GFXDevice::instance().toggle2D(true);
}

Scoped2DRendering::~Scoped2DRendering() {
    GFXDevice::instance().toggle2D(false);
}

ScopedViewport::ScopedViewport(const vec4<I32>& viewport) {
    GFXDevice::instance().setViewport(viewport);
}

ScopedViewport::~ScopedViewport() {
    GFXDevice::instance().restoreViewport();
}

};  // namespace GFX
};  // namespace Divide