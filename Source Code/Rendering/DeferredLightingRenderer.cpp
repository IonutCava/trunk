#include "Headers/DeferredLightingRenderer.h"
#include "Managers/Headers/RenderPassManager.h"

DeferredLightingRenderer::DeferredLightingRenderer() : Renderer(RENDERER_DEFERRED_LIGHTING)
{
}

DeferredLightingRenderer::~DeferredLightingRenderer()
{
}

void DeferredLightingRenderer::render(const DELEGATE_CBK& renderCallback, const SceneRenderState& sceneRenderState) {
	renderCallback();
}