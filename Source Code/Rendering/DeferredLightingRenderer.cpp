#include "Headers/DeferredLightingRenderer.h"
#include "Managers/Headers/RenderPassManager.h"

DeferredLightingRenderer::DeferredLightingRenderer() : Renderer(RENDERER_DEFERRED_LIGHTING)
{
}

DeferredLightingRenderer::~DeferredLightingRenderer()
{
}

void DeferredLightingRenderer::render(boost::function0<void> renderCallback, const SceneRenderState& sceneRenderState) {
	renderCallback();
	RenderPassManager::getInstance().render(sceneRenderState);
}