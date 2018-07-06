#include "Headers/ForwardPlusRenderer.h"

#include "Core/Headers/Console.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

ForwardPlusRenderer::ForwardPlusRenderer()
    : Renderer(RendererType::RENDERER_FORWARD_PLUS) {
    _opaqueGrid.reset(new LightGrid());
    _transparentGrid.reset(new LightGrid());
    /// Initialize our depth ranges construction shader (see LightManager.cpp
    /// for more documentation)
    ResourceDescriptor rangesDesc("DepthRangesConstruct");

    stringImpl gridDim("LIGHT_GRID_TILE_DIM_X ");
    gridDim.append(
        std::to_string(Config::Lighting::LIGHT_GRID_TILE_DIM_X));
    gridDim.append(",LIGHT_GRID_TILE_DIM_Y ");
    gridDim.append(
        std::to_string(Config::Lighting::LIGHT_GRID_TILE_DIM_Y));
    rangesDesc.setPropertyList(gridDim);

    _depthRangesConstructProgram = CreateResource<ShaderProgram>(rangesDesc);
    _depthRangesConstructProgram->Uniform("depthTex", ShaderProgram::TextureUsage::UNIT0);
    /// Depth ranges are used for grid based light culling
    SamplerDescriptor depthRangesSampler;
    depthRangesSampler.setFilters(TextureFilter::NEAREST);
    depthRangesSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    TextureDescriptor depthRangesDescriptor(TextureType::TEXTURE_2D,
                                            GFXImageFormat::RGBA32F,
                                            GFXDataFormat::FLOAT_32);
    depthRangesDescriptor.setSampler(depthRangesSampler);
    // The down-sampled depth buffer is used to cull screen space lights for our
    // Forward+ rendering algorithm.
    // It's only updated on demand.
    _depthRanges = GFX_DEVICE.newFB(false);
    _depthRanges->AddAttachment(depthRangesDescriptor,
                                TextureDescriptor::AttachmentType::Color0);
    _depthRanges->toggleDepthBuffer(false);
    _depthRanges->setClearColor(vec4<F32>(0.0f, 1.0f, 0.0f, 1.0f));
    vec2<U16> screenRes =
        GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
            ->getResolution();
    vec2<U16> tileSize(Config::Lighting::LIGHT_GRID_TILE_DIM_X,
                       Config::Lighting::LIGHT_GRID_TILE_DIM_Y);
    vec2<U16> resTemp(screenRes + tileSize);
    _depthRanges->Create(resTemp.x / tileSize.x - 1,
                         resTemp.y / tileSize.y - 1);
}

ForwardPlusRenderer::~ForwardPlusRenderer() {
    MemoryManager::DELETE(_depthRanges);
    RemoveResource(_depthRangesConstructProgram);
}

void ForwardPlusRenderer::preRender(const GFXDevice::GPUBlock& gpuBlock) {
    buildLightGrid(gpuBlock);
}

void ForwardPlusRenderer::render(const DELEGATE_CBK<>& renderCallback,
                                 const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void ForwardPlusRenderer::updateResolution(U16 width, U16 height) {
    vec2<U16> tileSize(Config::Lighting::LIGHT_GRID_TILE_DIM_X,
                       Config::Lighting::LIGHT_GRID_TILE_DIM_Y);
    vec2<U16> resTemp(
        GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
            ->getResolution() +
        tileSize);
    _depthRanges->Create(resTemp.x / tileSize.x - 1,
                         resTemp.y / tileSize.y - 1);
}

bool ForwardPlusRenderer::buildLightGrid(const GFXDevice::GPUBlock& gpuBlock) {
    if (GFX_DEVICE.getRenderStage() != RenderStage::DISPLAY) {
        return true;
    }
    const Light::LightList& lights = LightManager::getInstance().getLights();

    _omniLightList.clear();
    _omniLightList.reserve(static_cast<vectorAlg::vecSize>(lights.size()));

    for (Light* const light : lights) {
        if (light->getLightType() == LightType::POINT) {
            _omniLightList.push_back(LightGrid::makeLight(
                light->getPosition(), light->getDiffuseColor(),
                light->getRange()));
        }
    }

    if (!_omniLightList.empty()) {
        _transparentGrid->build(
            vec2<U16>(Config::Lighting::LIGHT_GRID_TILE_DIM_X,
                      Config::Lighting::LIGHT_GRID_TILE_DIM_Y),
            // render target resolution
            vec2<U16>(gpuBlock._ViewPort.z, gpuBlock._ViewPort.w),
            _omniLightList, gpuBlock._ViewMatrix, gpuBlock._ProjectionMatrix,
            // current near plane
            gpuBlock._ZPlanesCombined.x, vectorImpl<vec2<F32>>());

        downSampleDepthBuffer(_depthRangesCache);
        // We take a copy of this, and prune the grid using depth ranges found
        // from pre-z pass
        // (for opaque geometry).
        STUBBED("ADD OPTIMIZED COPY!!!");
        *_opaqueGrid = *_transparentGrid;
        // Note that the pruning does not occur if the pre-z pass was not
        // performed
        // (depthRanges is empty in this case).
        _opaqueGrid->prune(_depthRangesCache);
        _transparentGrid->pruneFarOnly(gpuBlock._ZPlanesCombined.x,
                                       _depthRangesCache);
        return true;
    }

    return false;
}

/// Extract the depth ranges and store them in a different render target used to
/// cull lights against
void ForwardPlusRenderer::downSampleDepthBuffer(
    vectorImpl<vec2<F32>>& depthRanges) {
    depthRanges.resize(_depthRanges->getWidth() * _depthRanges->getHeight());

    _depthRanges->Begin(Framebuffer::defaultPolicy());
    {
        _depthRangesConstructProgram->bind();
        _depthRangesConstructProgram->Uniform(
            "dvd_ProjectionMatrixInverse",
            GFX_DEVICE.getMatrix(MATRIX_MODE::PROJECTION_INV));
        GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
            ->Bind(static_cast<U8>(ShaderProgram::TextureUsage::UNIT0),
                   TextureDescriptor::AttachmentType::Depth);
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true),
                              _depthRangesConstructProgram);

        _depthRanges->ReadData(GFXImageFormat::RG, GFXDataFormat::FLOAT_32,
                               &depthRanges[0]);
    }
    _depthRanges->End();
}
};