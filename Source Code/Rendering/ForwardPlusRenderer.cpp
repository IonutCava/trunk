#include "Headers/ForwardPlusRenderer.h"

#include "Core/Headers/Console.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Resources/Headers/ResourceCache.h"
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

    vec2<U16> res(GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->getResolution());
    updateResolution(res.width, res.height);

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
    Renderer::preRender();

    LightManager& lightMgr = LightManager::getInstance();
    lightMgr.uploadLightData(LightType::POINT, ShaderBufferLocation::LIGHT_CULL_POINT);
    lightMgr.uploadLightData(LightType::SPOT, ShaderBufferLocation::LIGHT_CULL_SPOT);

    Framebuffer* depthBuffer = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::DEPTH);
    Texture* depthTexture = depthBuffer->getAttachment(TextureDescriptor::AttachmentType::Depth);
    depthTexture->Bind(to_ubyte(ShaderProgram::TextureUsage::DEPTH));

    _flag = getMaxNumLightsPerTile();
    _lightCullComputeShader->bind();

    _lightCullComputeShader->Uniform("maxNumLightsPerTile", _flag);
    _lightCullComputeShader->DispatchCompute(getNumTilesX(), getNumTilesY(), 1);
    _lightCullComputeShader->SetMemoryBarrier(ShaderProgram::MemoryBarrierType::SHADER_BUFFER);

    if (false)  {
        const U32 numTiles = getNumTilesX() * getNumTilesY();
        const U32 maxNumLightsPerTile = getMaxNumLightsPerTile();

        vectorImpl<U32> data;
        data.resize(numTiles * maxNumLightsPerTile);
        _perTileLightIndexBuffer->getData(0, data.size(), &data[0]);

        U32 tileCount = 0;
        Console::togglethreadID(false);
        Console::toggleTimeStamps(false);
        Console::printfn("\n ------------ START TILE %d -------------", tileCount);
        U32 count = 0;
        for (U32 c : data) {
            Console::printf("%d ", c);
            if (count++ % 50 == 0) {
                Console::printfn("\n");
            }
        }
        Console::printfn("\n -------------------- END TILE %d ---------------", tileCount++);
        Console::toggleTimeStamps(true);
        Console::togglethreadID(true);
    }
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
    U16 height = std::min(_resolution.height, to_ushort(1080));
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