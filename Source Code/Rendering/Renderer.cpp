#include "Headers/Renderer.h"

#include "Core/Headers/PlatformContext.h"

namespace Divide {

Renderer::Renderer(PlatformContext& context, ResourceCache& cache, RendererType type)
    : _context(context),
      _resCache(cache),
      _flag(0),
      _debugView(false),
      _type(type)
{
}

Renderer::~Renderer()
{
}

void Renderer::preRender(RenderTarget& target, LightPool& lightPool) {
    Attorney::GFXDeviceRenderer::uploadGPUBlock(_context.gfx());
}
};