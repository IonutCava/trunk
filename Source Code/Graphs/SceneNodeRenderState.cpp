#include "Headers/SceneNodeRenderState.h"
#include "Hardware/Video/Headers/GFXDevice.h"

SceneNodeRenderState::~SceneNodeRenderState()
{
    SAFE_DELETE(_depthStateBlock);
    SAFE_DELETE(_shadowStateBlock);
}

RenderStateBlock* SceneNodeRenderState::getDepthStateBlock(){
    if(!_depthStateBlock){
        RenderStateBlockDescriptor depthDesc;
        depthDesc.setColorWrites(false,false,false,false);
        _depthStateBlock = GFX_DEVICE.createStateBlock(depthDesc);
    }
    return _depthStateBlock;
}

RenderStateBlock* SceneNodeRenderState::getShadowStateBlock(){
    if(!_shadowStateBlock){
        RenderStateBlockDescriptor depthDesc;
        /// Cull back faces for shadow rendering
        depthDesc.setCullMode(CULL_MODE_CCW);
        depthDesc._zBias = 1.1f;
        depthDesc.setColorWrites(true,true,false,false);
        _shadowStateBlock = GFX_DEVICE.createStateBlock(depthDesc);
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