#include "Headers/ForwardPlusRenderer.h"

#include "Managers/Headers/LightManager.h"

ForwardPlusRenderer::ForwardPlusRenderer() : Renderer(RENDERER_FORWARD_PLUS)
{
    _opaqueGrid = New LightGrid();
    _transparentGrid = New LightGrid();
}

ForwardPlusRenderer::~ForwardPlusRenderer()
{
    SAFE_DELETE(_opaqueGrid);
    SAFE_DELETE(_transparentGrid);
}

void ForwardPlusRenderer::processVisibleNodes(const vectorImpl<SceneGraphNode* >& visibleNodes, const GFXDevice::GPUBlock& gpuBlock) {
    buildLightGrid(gpuBlock);
}

void ForwardPlusRenderer::render(const DELEGATE_CBK& renderCallback, const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void ForwardPlusRenderer::updateResolution(U16 width, U16 height) {

}

bool ForwardPlusRenderer::buildLightGrid(const GFXDevice::GPUBlock& gpuBlock) {
    const Light::LightMap& lights = LightManager::getInstance().getLights();

    _omniLightList.resize(0);
    _omniLightList.reserve(lights.size());

    FOR_EACH(const Light::LightMap::value_type& it, lights) {
        const Light& light = *it.second;
        if (light.getLightType() == LIGHT_TYPE_POINT){
            _omniLightList.push_back(LightGrid::make_light(light.getPosition(), light.getDiffuseColor(), light.getRange()));
        }
    }

    if (!_omniLightList.empty()) {
        _transparentGrid->build(vec2<U16>(Config::Lighting::LIGHT_GRID_TILE_DIM_X, Config::Lighting::LIGHT_GRID_TILE_DIM_Y),
                                vec2<U16>(gpuBlock._ViewPort.z, gpuBlock._ViewPort.w), //< render target resolution
                                _omniLightList,
                                gpuBlock._ViewMatrix,
                                gpuBlock._ProjectionMatrix,
                                gpuBlock._ZPlanesCombined.x, //< current near plane
                                vectorImpl<vec2<F32> >());

         
        GFX_DEVICE.DownSampleDepthBuffer(_depthRangesCache);
        // We take a copy of this, and prune the grid using depth ranges found from pre-z pass (for opaque geometry).
        _opaqueGrid = _transparentGrid;
        // Note that the pruning does not occur if the pre-z pass was not performed (depthRanges is empty in this case).
        _opaqueGrid->prune(_depthRangesCache);
        _transparentGrid->pruneFarOnly(gpuBlock._ZPlanesCombined.x, _depthRangesCache);
        return true;
    }

    return false;
}