#include "Headers/ScopedStates.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
namespace GFX {

Scoped2DRendering::Scoped2DRendering(bool state) {
    _2dRenderingState = GFX_DEVICE.is2DRendering();
    GFX_DEVICE.toggle2D(state);
}

Scoped2DRendering::~Scoped2DRendering() {
    GFX_DEVICE.toggle2D(_2dRenderingState);
}

ScopedViewport::ScopedViewport(const vec4<I32>& viewport) {
    GFX_DEVICE.setViewport(viewport);
}

ScopedViewport::~ScopedViewport() {
    GFX_DEVICE.restoreViewport();
}

};  // namespace GFX
};  // namespace Divide