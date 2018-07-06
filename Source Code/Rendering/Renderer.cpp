#include "Headers/Renderer.h"

#include "Core/Headers/PlatformContext.h"
#include "Rendering/Lighting/Headers/LightPool.h"

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
    lightPool.uploadLightData(ShaderBufferLocation::LIGHT_NORMAL,
                              ShaderBufferLocation::LIGHT_SHADOW);

    Attorney::GFXDeviceRenderer::uploadGPUBlock(_context.gfx());
}
};