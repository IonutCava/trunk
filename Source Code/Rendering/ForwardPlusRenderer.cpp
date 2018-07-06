#include "Headers/ForwardPlusRenderer.h"

#include "Core/Headers/Console.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

//ref: https://github.com/bioglaze/aether3d
namespace Divide {
namespace {
    I32 numActivePointLights = 0;
    I32 numActiveSpotLights = 0;

    U32 getMaxNumLightsPerTile() {
        const U32 adjustmentMultipier = 32;

        // I haven't tested at greater than 1080p, so cap it
        U16 height = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().height;
        CLAMP<U16>(height, height, 1080);

        // adjust max lights per tile down as height increases
        return (Config::Lighting::FORWARD_PLUS_MAX_LIGHTS_PER_TILE - (adjustmentMultipier * (height / 120)));
    }

    U32 getNumTilesX() { 
        U16 width = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().width;
        return to_uint((width + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) / to_float(Config::Lighting::FORWARD_PLUS_TILE_RES));
    }

    U32 getNumTilesY() { 
        U16 height = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().height;
        return to_uint((height + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) / to_float(Config::Lighting::FORWARD_PLUS_TILE_RES));
    }

    I32 getNextPointLightBufferIndex()
    {
        if (numActivePointLights + numActiveSpotLights < Config::Lighting::MAX_POSSIBLE_LIGHTS)
        {
            ++numActivePointLights;
            return numActivePointLights - 1;
        }

        // Error handling is done in the caller.
        return -1;
    }

    I32 getNextSpotLightBufferIndex()
    {
        if (numActiveSpotLights + numActiveSpotLights + 1 < Config::Lighting::MAX_POSSIBLE_LIGHTS)
        {
            ++numActiveSpotLights;
            return numActiveSpotLights - 1;
        }

        // Error handling is done in the caller.
        return -1;
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

    _perTileLightIndexBuffer.reset(GFX_DEVICE.newSB("dvd_perTileLightIndexBuffer", 1, true, false, BufferUpdateFrequency::OFTEN));
    _perTileLightIndexBuffer->create(4 * maxNumLightsPerTile * numTiles, sizeof(U32));

    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
}

ForwardPlusRenderer::~ForwardPlusRenderer()
{
    RemoveResource(_lightCullComputeShader);
}

void ForwardPlusRenderer::preRender() {
    LightManager& lightMgr = LightManager::getInstance();
    const vectorImpl<Light*>& pointLights = lightMgr.getLights(LightType::POINT);
    const vectorImpl<Light*>& spotLights = lightMgr.getLights(LightType::SPOT);
    numActivePointLights = to_int(pointLights.size());
    numActiveSpotLights = to_int(spotLights.size());

    lightMgr.uploadLightData(LightType::POINT, ShaderBufferLocation::LIGHT_CULL_POINT);
    lightMgr.uploadLightData(LightType::SPOT, ShaderBufferLocation::LIGHT_CULL_SPOT);

    GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
        ->getAttachment(TextureDescriptor::AttachmentType::Depth)
        ->Bind(to_ubyte(ShaderProgram::TextureUsage::DEPTH));

    _flag = getMaxNumLightsPerTile();
    _lightCullComputeShader->bind();
    _lightCullComputeShader->Uniform("invProjection", GFX_DEVICE.getMatrix(MATRIX::PROJECTION_INV));
    _lightCullComputeShader->Uniform("numLights", (((U32)numActiveSpotLights & 0xFFFFu) << 16) | ((U32)numActivePointLights & 0xFFFFu));
    _lightCullComputeShader->Uniform("maxNumLightsPerTile", (I32)_flag);
    _lightCullComputeShader->DispatchCompute(getNumTilesX(), getNumTilesY(), 1);
    _lightCullComputeShader->SetMemoryBarrier();

    //const U32 numTiles = getNumTilesX() * getNumTilesY();
    //const U32 maxNumLightsPerTile = getMaxNumLightsPerTile();
    //vectorImpl<U32> data(4 * maxNumLightsPerTile * numTiles);
    //_perTileLightIndexBuffer->getData(0, 4 * maxNumLightsPerTile * numTiles, &data[0]);
}

void ForwardPlusRenderer::render(const DELEGATE_CBK<>& renderCallback,
                                 const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void ForwardPlusRenderer::updateResolution(U16 width, U16 height) {

}
};