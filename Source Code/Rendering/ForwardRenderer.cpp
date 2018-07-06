#include "Headers/ForwardRenderer.h"
#include "Managers/Headers/RenderPassManager.h"

ForwardRenderer::ForwardRenderer() : Renderer(RENDERER_FORWARD)
{
}

ForwardRenderer::~ForwardRenderer()
{
}

void ForwardRenderer::render(boost::function0<void> renderCallback, const SceneRenderState& sceneRenderState) {
	renderCallback();
}