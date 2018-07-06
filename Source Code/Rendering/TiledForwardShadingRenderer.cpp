#include "stdafx.h"

#include "config.h"

#include "Headers/TiledForwardShadingRenderer.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

//ref: https://github.com/bioglaze/aether3d
namespace Divide {

TiledForwardShadingRenderer::TiledForwardShadingRenderer(PlatformContext& context, ResourceCache& cache)
    : Renderer(context, cache, RendererType::RENDERER_TILED_FORWARD_SHADING)
{
    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.setThreadedLoading(false);
    _lightCullComputeShader = CreateResource<ShaderProgram>(cache, cullShaderDesc);

    RenderTarget& screenRT = _context.gfx().renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));

    updateResolution(screenRT.getWidth(), screenRT.getHeight());

    const U32 numTiles = getNumTilesX() * getNumTilesY();
    const U32 maxNumLightsPerTile = getMaxNumLightsPerTile();

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._primitiveCount = maxNumLightsPerTile * numTiles;
    bufferDescriptor._primitiveSizeInBytes = sizeof(U32);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._unbound = true;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;

    _perTileLightIndexBuffer = _context.gfx().newSB(bufferDescriptor);
    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
}

TiledForwardShadingRenderer::~TiledForwardShadingRenderer()
{
}

void TiledForwardShadingRenderer::preRender(RenderTarget& target, LightPool& lightPool) {
    Renderer::preRender(target, lightPool);

    target.bind(to_U8(ShaderProgram::TextureUsage::DEPTH), RTAttachmentType::Depth, 0);

    _flag = getMaxNumLightsPerTile();
    _lightCullComputeShader->bind();

    PushConstants constants;
    constants.set("maxNumLightsPerTile", PushConstantType::UINT, _flag);

    _lightCullComputeShader->DispatchCompute(getNumTilesX(), getNumTilesY(), 1, constants);
    _lightCullComputeShader->SetMemoryBarrier(ShaderProgram::MemoryBarrierType::SHADER_BUFFER);
}

void TiledForwardShadingRenderer::render(const DELEGATE_CBK<void>& renderCallback,
                                 const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void TiledForwardShadingRenderer::updateResolution(U16 width, U16 height) {
    _resolution.set(width, height);
}

U32 TiledForwardShadingRenderer::getMaxNumLightsPerTile() const {
    const U32 adjustmentMultipier = 32;

    // I haven't tested at greater than 1080p, so cap it
    U16 height = std::min(_resolution.height, to_U16(1080));
    // adjust max lights per tile down as height increases
    return (Config::Lighting::FORWARD_PLUS_MAX_LIGHTS_PER_TILE - (adjustmentMultipier * (height / 120)));
}

U32 TiledForwardShadingRenderer::getNumTilesX() const {
    return to_U32((_resolution.width + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) /
           to_F32(Config::Lighting::FORWARD_PLUS_TILE_RES));
}

U32 TiledForwardShadingRenderer::getNumTilesY() const {
    return to_U32((_resolution.height + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) /
           to_F32(Config::Lighting::FORWARD_PLUS_TILE_RES));
}
};