#include "Headers/ForwardPlusRenderer.h"

#include "Core/Headers/Console.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

//ref: https://github.com/bioglaze/aether3d
namespace Divide {

ForwardPlusRenderer::ForwardPlusRenderer() 
    : Renderer(RendererType::RENDERER_FORWARD_PLUS)
{
    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.setThreadedLoading(false);
    _lightCullComputeShader = CreateResource<ShaderProgram>(cullShaderDesc);

    vec2<U16> res(GFX_DEVICE.getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer->getResolution());
    updateResolution(res.width, res.height);

    const U32 numTiles = getNumTilesX() * getNumTilesY();
    const U32 maxNumLightsPerTile = getMaxNumLightsPerTile();

    _perTileLightIndexBuffer.reset(GFX_DEVICE.newSB("dvd_perTileLightIndexBuffer", 1, true, true, BufferUpdateFrequency::ONCE));
    _perTileLightIndexBuffer->create(maxNumLightsPerTile * numTiles, sizeof(U32));
    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
}

ForwardPlusRenderer::~ForwardPlusRenderer()
{
}

void ForwardPlusRenderer::preRender(LightPool& lightPool) {
    Renderer::preRender(lightPool);

    lightPool.uploadLightData(ShaderBufferLocation::LIGHT_NORMAL);

    GFX_DEVICE.getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer
        ->bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH),
               TextureDescriptor::AttachmentType::Depth);

    _flag = getMaxNumLightsPerTile();
    _lightCullComputeShader->bind();

    _lightCullComputeShader->Uniform("maxNumLightsPerTile", _flag);
    _lightCullComputeShader->DispatchCompute(getNumTilesX(), getNumTilesY(), 1);
    _lightCullComputeShader->SetMemoryBarrier(ShaderProgram::MemoryBarrierType::SHADER_BUFFER);
}

void ForwardPlusRenderer::render(const DELEGATE_CBK<>& renderCallback,
                                 const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void ForwardPlusRenderer::updateResolution(U16 width, U16 height) {
    _resolution.set(width, height);
}

U32 ForwardPlusRenderer::getMaxNumLightsPerTile() const {
    const U32 adjustmentMultipier = 32;

    // I haven't tested at greater than 1080p, so cap it
    U16 height = std::min(_resolution.height, to_const_ushort(1080));
    // adjust max lights per tile down as height increases
    return (Config::Lighting::FORWARD_PLUS_MAX_LIGHTS_PER_TILE - (adjustmentMultipier * (height / 120)));
}

U32 ForwardPlusRenderer::getNumTilesX() const {
    return to_uint((_resolution.width + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) /
           to_float(Config::Lighting::FORWARD_PLUS_TILE_RES));
}

U32 ForwardPlusRenderer::getNumTilesY() const {
    return to_uint((_resolution.height + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) /
           to_float(Config::Lighting::FORWARD_PLUS_TILE_RES));
}
};