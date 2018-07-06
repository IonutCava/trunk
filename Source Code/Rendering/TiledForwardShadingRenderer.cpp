#include "config.h"

#include "Headers/TiledForwardShadingRenderer.h"

#include "Core/Headers/Console.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

//ref: https://github.com/bioglaze/aether3d
namespace Divide {

TiledForwardShadingRenderer::TiledForwardShadingRenderer() 
    : Renderer(RendererType::RENDERER_TILED_FORWARD_SHADING)
{
    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.setThreadedLoading(false);
    _lightCullComputeShader = CreateResource<ShaderProgram>(cullShaderDesc);

    updateResolution(GFX_DEVICE.renderTarget(RenderTargetID::SCREEN).getWidth(),
                     GFX_DEVICE.renderTarget(RenderTargetID::SCREEN).getHeight());

    const U32 numTiles = getNumTilesX() * getNumTilesY();
    const U32 maxNumLightsPerTile = getMaxNumLightsPerTile();

    _perTileLightIndexBuffer.reset(GFX_DEVICE.newSB(1, true, true, BufferUpdateFrequency::ONCE));
    _perTileLightIndexBuffer->create(maxNumLightsPerTile * numTiles, sizeof(U32));
    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
}

TiledForwardShadingRenderer::~TiledForwardShadingRenderer()
{
}

void TiledForwardShadingRenderer::preRender(RenderTarget& target, LightPool& lightPool) {
    Renderer::preRender(target, lightPool);

    lightPool.uploadLightData(ShaderBufferLocation::LIGHT_NORMAL);

    target.bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH), RTAttachment::Type::Depth, 0);

    _flag = getMaxNumLightsPerTile();
    _lightCullComputeShader->bind();

    _lightCullComputeShader->Uniform("maxNumLightsPerTile", _flag);
    _lightCullComputeShader->DispatchCompute(getNumTilesX(), getNumTilesY(), 1);
    _lightCullComputeShader->SetMemoryBarrier(ShaderProgram::MemoryBarrierType::SHADER_BUFFER);
}

void TiledForwardShadingRenderer::render(const DELEGATE_CBK<>& renderCallback,
                                 const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void TiledForwardShadingRenderer::updateResolution(U16 width, U16 height) {
    _resolution.set(width, height);
}

U32 TiledForwardShadingRenderer::getMaxNumLightsPerTile() const {
    const U32 adjustmentMultipier = 32;

    // I haven't tested at greater than 1080p, so cap it
    U16 height = std::min(_resolution.height, to_const_ushort(1080));
    // adjust max lights per tile down as height increases
    return (Config::Lighting::FORWARD_PLUS_MAX_LIGHTS_PER_TILE - (adjustmentMultipier * (height / 120)));
}

U32 TiledForwardShadingRenderer::getNumTilesX() const {
    return to_uint((_resolution.width + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) /
           to_float(Config::Lighting::FORWARD_PLUS_TILE_RES));
}

U32 TiledForwardShadingRenderer::getNumTilesY() const {
    return to_uint((_resolution.height + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) /
           to_float(Config::Lighting::FORWARD_PLUS_TILE_RES));
}
};