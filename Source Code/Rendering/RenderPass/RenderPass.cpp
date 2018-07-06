#include "Headers/RenderPass.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

RenderPass::RenderPass(const std::string& name) : _name(name)
{
	_lastTotalBinSize = 0;
}

RenderPass::~RenderPass()
{
}

void RenderPass::render(const SceneRenderState& renderState) {
	const RenderStage& currentStage = GFX_DEVICE.getRenderStage();
		  RenderQueue& renderQueue = RenderQueue::getInstance();
	//Sort the render queue by the specified key
	renderQueue.sort();
	U16 renderBinCount   = renderQueue.getRenderQueueBinSize();
	   _lastTotalBinSize = renderQueue.getRenderQueueStackSize();
	//Draw the entire queue;
	//Limited to 65536 (2^16) items per queue pass!
    if(renderState.drawObjects()){
		for(U16 i = 0; i < renderBinCount; i++){
			renderQueue.getBinSorted(i)->render(currentStage);
		}
    }
	//Unbind all shaders after every render pass
	ShaderManager::getInstance().unbind();

	if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
		for(U16 i = 0; i < renderBinCount; i++){
			renderQueue.getBinSorted(i)->postRender();
		}
        SceneGraphNode* root = GET_ACTIVE_SCENEGRAPH()->getRoot();
        root->getNode<SceneNode>()->preFrameDrawEnd(root);
	}

	renderQueue.refresh();
}