#include "Headers/ForwardPlusRenderer.h"

#include "Core/Headers/Console.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

//ref: https://github.com/bioglaze/aether3d
namespace Divide {
namespace {
    struct PointLightData {
        vec4<F32> posAndCenter;
        vec4<F32> color;
    };

    struct SpotLightData : public PointLightData {
        vec4<F32> params;
    };

    const U32 MAX_NUM_LIGHTS_PER_TILE = 544;
    const I32 TILE_RES = 16;
    I32 numActivePointLights = 0;
    I32 numActiveSpotLights = 0;
    std::array<PointLightData, Config::Lighting::NUM_POSSIBLE_LIGHTS> pointLightData;
    std::array<SpotLightData, Config::Lighting::NUM_POSSIBLE_LIGHTS> spotLightData;

    U32 getMaxNumLightsPerTile() {
        const U32 adjustmentMultipier = 32;

        // I haven't tested at greater than 1080p, so cap it
        U16 height = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().height;
        CLAMP<U16>(height, height, 1080);

        // adjust max lights per tile down as height increases
        return (MAX_NUM_LIGHTS_PER_TILE - (adjustmentMultipier * (height / 120)));
    }

    U32 getNumTilesX() { 
        U16 width = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().width;
        return to_uint((width + TILE_RES - 1) / to_float(TILE_RES));
    }

    U32 getNumTilesY() { 
        U16 height = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution().height;
        return to_uint((height + TILE_RES - 1) / to_float(TILE_RES));
    }

    I32 getNextPointLightBufferIndex()
    {
        if (numActivePointLights + numActiveSpotLights < Config::Lighting::NUM_POSSIBLE_LIGHTS)
        {
            ++numActivePointLights;
            return numActivePointLights - 1;
        }

        // Error handling is done in the caller.
        return -1;
    }

    I32 getNextSpotLightBufferIndex()
    {
        if (numActiveSpotLights + numActiveSpotLights + 1 < Config::Lighting::NUM_POSSIBLE_LIGHTS)
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

    _pointLightBuffer.reset(GFX_DEVICE.newSB("dvd_pointLightBuffer", 1, true, false, BufferUpdateFrequency::OFTEN));
    _pointLightBuffer->create(Config::Lighting::NUM_POSSIBLE_LIGHTS, sizeof(PointLightData));

    _spotLightBuffer.reset(GFX_DEVICE.newSB("dvd_spotLightBuffer", 1, true, false, BufferUpdateFrequency::OFTEN));
    _spotLightBuffer->create(Config::Lighting::NUM_POSSIBLE_LIGHTS, sizeof(SpotLightData));

    const U32 numTiles = getNumTilesX() * getNumTilesY();
    const U32 maxNumLightsPerTile = getMaxNumLightsPerTile();

    _perTileLightIndexBuffer.reset(GFX_DEVICE.newSB("dvd_perTileLightIndexBuffer", 1, true, false, BufferUpdateFrequency::OFTEN));
    _perTileLightIndexBuffer->create(4 * maxNumLightsPerTile * numTiles, sizeof(U32));

    _pointLightBuffer->bind(ShaderBufferLocation::LIGHT_POINT_LIGHTS);
    _spotLightBuffer->bind(ShaderBufferLocation::LIGHT_SPOT_LIGHTS);
    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
}

ForwardPlusRenderer::~ForwardPlusRenderer()
{
    RemoveResource(_lightCullComputeShader);
}

void ForwardPlusRenderer::preRender() {
    const vectorImpl<Light*>& pointLights = LightManager::getInstance().getLights(LightType::POINT);
    const vectorImpl<Light*>& spotLights = LightManager::getInstance().getLights(LightType::SPOT);
    numActivePointLights = to_int(pointLights.size());
    numActiveSpotLights = to_int(spotLights.size());

    for (I32 i = 0; i < numActivePointLights; ++i) {
        PointLightData& data = pointLightData[i];
        Light* light = pointLights[i];
        data.posAndCenter.set(light->getPosition(), light->getRange());
        data.color.set(light->getDiffuseColor(), 1.0f);
    }
    _pointLightBuffer->setData(pointLightData.data());

    for (I32 i = 0; i < numActiveSpotLights; ++i) {
        SpotLightData& data = spotLightData[i];
        Light* light = spotLights[i];
        data.posAndCenter.set(light->getPosition(), light->getRange());
        data.color.set(light->getDiffuseColor());
        data.params.set(light->getDirection(), light->getProperties()._direction.w);
    }

    GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
        ->getAttachment(TextureDescriptor::AttachmentType::Depth)
        ->Bind(to_ubyte(ShaderProgram::TextureUsage::DEPTH));
    _spotLightBuffer->setData(spotLightData.data());

    _lightCullComputeShader->bind();
    _lightCullComputeShader->Uniform("invProjection", GFX_DEVICE.getMatrix(MATRIX_MODE::PROJECTION_INV));
    _lightCullComputeShader->Uniform("numLights", (((U32)numActiveSpotLights & 0xFFFFu) << 16) | ((U32)numActivePointLights & 0xFFFFu));
    _lightCullComputeShader->Uniform("maxNumLightsPerTile", (I32)getMaxNumLightsPerTile());
    _lightCullComputeShader->DispatchCompute(getNumTilesX(), getNumTilesY(), 1);
    _lightCullComputeShader->SetMemoryBarrier();
}

void ForwardPlusRenderer::render(const DELEGATE_CBK<>& renderCallback,
                                 const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void ForwardPlusRenderer::updateResolution(U16 width, U16 height) {

}
};