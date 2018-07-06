#include "Headers/RenderPass.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

namespace Divide {

RenderPass::RenderPass(const stringImpl& name) : _name(name)
{
    _lastTotalBinSize = 0;
}

RenderPass::~RenderPass() 
{
}

void RenderPass::render(const SceneRenderState& renderState) {
    RenderStage currentStage = GFX_DEVICE.getRenderStage();
    RenderQueue& renderQueue = RenderPassManager::getInstance().getQueue();

    if (currentStage == RenderStage::DISPLAY) {
        _lastTotalBinSize = renderQueue.getRenderQueueStackSize();
    }
    U16 renderBinCount = renderQueue.getRenderQueueBinSize();

    // Draw the entire queue;
    // Limited to 65536 (2^16) items per queue pass!
    if (renderState.drawGeometry()) {
        if ((currentStage == RenderStage::DISPLAY ||
             currentStage == RenderStage::REFLECTION) &&
            renderBinCount > 0) {
            LightManager::getInstance().bindShadowMaps();
            GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::DEPTH)->bind(0, 
                TextureDescriptor::AttachmentType::Depth);
        }

        for (U16 i = 0; i < renderBinCount; ++i) {
            renderQueue.getBinSorted(i)->render(renderState, currentStage);
        }
    }
    
    if (currentStage == RenderStage::DISPLAY) {
        for (U16 i = 0; i < renderBinCount; ++i) {
            renderQueue.getBinSorted(i)->postRender(renderState, currentStage);
        }
    }
}
};