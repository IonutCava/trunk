#include "Headers/ForwardPlusRenderer.h"

#include "Managers/Headers/LightManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

ForwardPlusRenderer::ForwardPlusRenderer() : Renderer(RENDERER_FORWARD_PLUS)
{
    _opaqueGrid = New LightGrid();
    _transparentGrid = New LightGrid();
    /// Initialize our depth ranges construction shader (see LightManager.cpp for more documentation)
    ResourceDescriptor rangesDesc("DepthRangesConstruct");
    rangesDesc.setPropertyList(stringAlg::toBase("LIGHT_GRID_TILE_DIM_X " + Util::toString(Config::Lighting::LIGHT_GRID_TILE_DIM_X) + "," +
                                                 "LIGHT_GRID_TILE_DIM_Y " + Util::toString(Config::Lighting::LIGHT_GRID_TILE_DIM_Y)));
    _depthRangesConstructProgram = CreateResource<ShaderProgram>(rangesDesc);
    _depthRangesConstructProgram->UniformTexture("depthTex", 0);
    /// Depth ranges are used for grid based light culling
    SamplerDescriptor depthRangesSampler;
    depthRangesSampler.setFilters(TEXTURE_FILTER_NEAREST);
    depthRangesSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthRangesSampler.toggleMipMaps(false);
    TextureDescriptor depthRangesDescriptor(TEXTURE_2D, RGBA32F, FLOAT_32);
    depthRangesDescriptor.setSampler(depthRangesSampler);
    // The down-sampled depth buffer is used to cull screen space lights for our Forward+ rendering algorithm. It's only updated on demand.
    _depthRanges = GFX_DEVICE.newFB(false);
    _depthRanges->AddAttachment(depthRangesDescriptor, TextureDescriptor::Color0);
    _depthRanges->toggleDepthBuffer(false);
    _depthRanges->setClearColor(vec4<F32>(0.0f, 1.0f, 0.0f, 1.0f));
    vec2<U16> screenRes = GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->getResolution();
    vec2<U16> tileSize(Config::Lighting::LIGHT_GRID_TILE_DIM_X, Config::Lighting::LIGHT_GRID_TILE_DIM_Y);
    vec2<U16> resTemp(screenRes + tileSize);
    _depthRanges->Create(resTemp.x / tileSize.x - 1, resTemp.y / tileSize.y - 1);
    _depthRangesConstructProgram->Uniform("dvd_screenDimension", screenRes);
}

ForwardPlusRenderer::~ForwardPlusRenderer()
{
    MemoryManager::SAFE_DELETE( _depthRanges );
    MemoryManager::SAFE_DELETE( _opaqueGrid );
    MemoryManager::SAFE_DELETE( _transparentGrid );
    RemoveResource(_depthRangesConstructProgram);
}

void ForwardPlusRenderer::processVisibleNodes(const vectorImpl<SceneGraphNode* >& visibleNodes, const GFXDevice::GPUBlock& gpuBlock) {
    buildLightGrid(gpuBlock);
}

void ForwardPlusRenderer::render(const DELEGATE_CBK<>& renderCallback, const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void ForwardPlusRenderer::updateResolution(U16 width, U16 height) {
    vec2<U16> tileSize(Config::Lighting::LIGHT_GRID_TILE_DIM_X, Config::Lighting::LIGHT_GRID_TILE_DIM_Y);
    vec2<U16> resTemp(GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->getResolution() + tileSize);
    _depthRanges->Create(resTemp.x / tileSize.x - 1, resTemp.y / tileSize.y - 1);
    _depthRangesConstructProgram->Uniform("dvd_screenDimension", vec2<U16>(width, height));
}

bool ForwardPlusRenderer::buildLightGrid(const GFXDevice::GPUBlock& gpuBlock) {
    const Light::LightMap& lights = LightManager::getInstance().getLights();

    _omniLightList.clear();
    _omniLightList.reserve(static_cast<vectorAlg::vecSize>(lights.size()));

    for ( const Light::LightMap::value_type it : lights ) {
        const Light& light = *it.second;
        if ( light.getLightType() == LIGHT_TYPE_POINT ) {
            _omniLightList.push_back( LightGrid::make_light( light.getPosition(), light.getDiffuseColor(), light.getRange() ) );
        }
    }

    if ( !_omniLightList.empty() ) {
        _transparentGrid->build(vec2<U16>(Config::Lighting::LIGHT_GRID_TILE_DIM_X, Config::Lighting::LIGHT_GRID_TILE_DIM_Y),
                                vec2<U16>(gpuBlock._ViewPort.z, gpuBlock._ViewPort.w), //< render target resolution
                                _omniLightList,
                                gpuBlock._ViewMatrix,
                                gpuBlock._ProjectionMatrix,
                                gpuBlock._ZPlanesCombined.x, //< current near plane
                                vectorImpl<vec2<F32> >());

         
        downSampleDepthBuffer(_depthRangesCache);
        // We take a copy of this, and prune the grid using depth ranges found from pre-z pass (for opaque geometry).
        _opaqueGrid = _transparentGrid;
        // Note that the pruning does not occur if the pre-z pass was not performed (depthRanges is empty in this case).
        _opaqueGrid->prune(_depthRangesCache);
        _transparentGrid->pruneFarOnly(gpuBlock._ZPlanesCombined.x, _depthRangesCache);
        return true;
    }

    return false;
}

/// Extract the depth ranges and store them in a different render target used to cull lights against
void ForwardPlusRenderer::downSampleDepthBuffer(vectorImpl<vec2<F32>> &depthRanges){
    depthRanges.resize(_depthRanges->getWidth() * _depthRanges->getHeight());

    _depthRanges->Begin(Framebuffer::defaultPolicy());
    {
        _depthRangesConstructProgram->bind();
        _depthRangesConstructProgram->Uniform("dvd_ProjectionMatrixInverse", GFX_DEVICE.getMatrix(PROJECTION_INV_MATRIX));
        GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Bind(ShaderProgram::TEXTURE_UNIT0, TextureDescriptor::Depth);    
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _depthRangesConstructProgram);

        _depthRanges->ReadData(RG, FLOAT_32, &depthRanges[0]);
    }
    _depthRanges->End();
}

};