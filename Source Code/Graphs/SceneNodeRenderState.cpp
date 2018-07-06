#include "Headers/SceneNodeRenderState.h"
#include "Hardware/Video/Headers/GFXDevice.h"

namespace Divide {

SceneNodeRenderState::~SceneNodeRenderState()
{
}

size_t SceneNodeRenderState::getDepthStateBlock(){
    if(_depthStateBlockHash == 0){
        RenderStateBlockDescriptor depthDesc;
        depthDesc.setColorWrites(false,false,false,false);
        _depthStateBlockHash = GFX_DEVICE.getOrCreateStateBlock(depthDesc);
    }
    return _depthStateBlockHash;
}

size_t SceneNodeRenderState::getShadowStateBlock(){
    if(_shadowStateBlockHash == 0){
        RenderStateBlockDescriptor depthDesc;
        /// Cull back faces for shadow rendering
        depthDesc.setCullMode(CULL_MODE_CCW);
        //depthDesc.setZBias(1.0f, 2.0f);
        depthDesc.setColorWrites(true,true,false,false);
        _shadowStateBlockHash = GFX_DEVICE.getOrCreateStateBlock(depthDesc);
    }
    return _shadowStateBlockHash;
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

};