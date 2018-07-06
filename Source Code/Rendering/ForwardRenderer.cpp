#include "Headers/ForwardRenderer.h"
#include "Managers/Headers/RenderPassManager.h"

ForwardRenderer::ForwardRenderer() : Renderer(RENDERER_FORWARD)
{
}

ForwardRenderer::~ForwardRenderer()
{
}

void ForwardRenderer::render(const DELEGATE_CBK& renderCallback, const SceneRenderState& sceneRenderState) {
	renderCallback();
}

void ForwardRenderer::updateResolution(U16 width, U16 height) {

}