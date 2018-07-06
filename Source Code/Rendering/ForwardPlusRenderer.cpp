#include "Headers/ForwardPlusRenderer.h"

#include "Core/Headers/Console.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

//ref: https://github.com/bioglaze/aether3d
namespace Divide {
namespace {
    U32 numActivePointLights = 0;
    U32 numActiveSpotLights = 0;

    U32 getMaxNumLightsPerTile() {
        const U32 adjustmentMultipier = 32;

        // I haven't tested at greater than 1080p, so cap it
        U16 height = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().height;
        CLAMP<U16>(height, height, 1080);

        // adjust max lights per tile down as height increases
        //return (Config::Lighting::FORWARD_PLUS_MAX_LIGHTS_PER_TILE - (adjustmentMultipier * (height / 120)));
        return 32;
    }

    U32 getNumTilesX() { 
        U16 width = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().width;
        return to_uint((width + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) / to_float(Config::Lighting::FORWARD_PLUS_TILE_RES));
    }

    U32 getNumTilesY() { 
        U16 height = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().height;
        return to_uint((height + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) / to_float(Config::Lighting::FORWARD_PLUS_TILE_RES));
    }
};

ForwardPlusRenderer::ForwardPlusRenderer() 
    : Renderer(RendererType::RENDERER_FORWARD_PLUS)
{
    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.setThreadedLoading(false);
    _lightCullComputeShader = CreateResource<ShaderProgram>(cullShaderDesc);

    const U32 numTiles = getNumTilesX() * getNumTilesY();
    const U32 maxNumLightsPerTile = getMaxNumLightsPerTile();

    _perTileLightIndexBuffer.reset(GFX_DEVICE.newSB("dvd_perTileLightIndexBuffer", 1, true, true, BufferUpdateFrequency::ONCE));
    _perTileLightIndexBuffer->create(maxNumLightsPerTile * numTiles, sizeof(U32));
    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
}

ForwardPlusRenderer::~ForwardPlusRenderer()
{
    RemoveResource(_lightCullComputeShader);
}

void ForwardPlusRenderer::preRender() {
    LightManager& lightMgr = LightManager::getInstance();
    numActivePointLights = lightMgr.getActiveLightCount(LightType::POINT);
    numActiveSpotLights = lightMgr.getActiveLightCount(LightType::SPOT);

    lightMgr.uploadLightData(LightType::POINT, ShaderBufferLocation::LIGHT_CULL_POINT);
    lightMgr.uploadLightData(LightType::SPOT, ShaderBufferLocation::LIGHT_CULL_SPOT);

    GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
        ->getAttachment(TextureDescriptor::AttachmentType::Depth)
        ->Bind(to_ubyte(ShaderProgram::TextureUsage::DEPTH));

    _flag = getMaxNumLightsPerTile();
    _lightCullComputeShader->bind();
    _lightCullComputeShader->Uniform("numLights", ((numActiveSpotLights & 0xFFFFu) << 16) |
                                                   (numActivePointLights & 0xFFFFu));
    _lightCullComputeShader->Uniform("maxNumLightsPerTile", (I32)_flag);
    _lightCullComputeShader->DispatchCompute(getNumTilesX(), getNumTilesY(), 1);
    _lightCullComputeShader->SetMemoryBarrier(ShaderProgram::MemoryBarrierType::BUFFER);
}

void ForwardPlusRenderer::render(const DELEGATE_CBK<>& renderCallback,
                                 const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void ForwardPlusRenderer::updateResolution(U16 width, U16 height) {

}
};