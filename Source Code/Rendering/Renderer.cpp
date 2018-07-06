#include "stdafx.h"

#include "Headers/Renderer.h"

#include "Core/Headers/PlatformContext.h"
#include "Rendering/Lighting/Headers/LightPool.h"

namespace Divide {

Renderer::Renderer(PlatformContext& context, ResourceCache& cache, RendererType type)
    : PlatformContextComponent(context),
      _resCache(cache),
      _flag(0),
      _debugView(false),
      _type(type)
{
}

Renderer::~Renderer()
{
}

void Renderer::preRender(RenderTarget& target,
                         LightPool& lightPool,
                         GFX::CommandBuffer& bufferInOut) {
    lightPool.uploadLightData(ShaderBufferLocation::LIGHT_NORMAL,
                              ShaderBufferLocation::LIGHT_SHADOW);
}
};