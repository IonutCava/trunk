#include "Headers/SceneNodeRenderState.h"
#include "Hardware/Video/Headers/GFXDevice.h"

SceneNodeRenderState::~SceneNodeRenderState()
{
}

RenderStateBlock* SceneNodeRenderState::getDepthStateBlock(){
    if(!_depthStateBlock){
        RenderStateBlockDescriptor depthDesc;
        depthDesc.setColorWrites(false,false,false,false);
        _depthStateBlock = GFX_DEVICE.getOrCreateStateBlock(depthDesc);
    }
    return _depthStateBlock;
}

RenderStateBlock* SceneNodeRenderState::getShadowStateBlock(){
    if(!_shadowStateBlock){
        RenderStateBlockDescriptor depthDesc;
        /// Cull back faces for shadow rendering
        depthDesc.setCullMode(CULL_MODE_CCW);
        depthDesc.setZBias(1.0f, 2.0f);
        depthDesc.setColorWrites(true,true,false,false);
        _shadowStateBlock = GFX_DEVICE.getOrCreateStateBlock(depthDesc);
    }
    return _shadowStateBlock;
}

void SceneNodeRenderState::removeFromDrawExclusionMask(U32 stageMask) {
    assert((stageMask & ~(INVALID_STAGE-1)) == 0);
    _exclusionMask &= ~stageMask;
}

void SceneNodeRenderState::addToDrawExclusionMask(U32 stageMask) {
    assert((stageMask & ~(INVALID_STAGE-1)) == 0);
    _exclusionMask |= static_cast<RenderStage>(stageMask);
}

bool SceneNodeRenderState::getDrawState(const RenderStage& currentStage)  const {
    return _drawState && !bitCompare(_exclusionMask, currentStage);
}