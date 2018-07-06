#include "Headers/SceneNodeRenderState.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

SceneNodeRenderState::~SceneNodeRenderState() {}

U32 SceneNodeRenderState::getDepthStateBlock() {
    if (_depthStateBlockHash == 0) {
        RenderStateBlock depthDesc;
        depthDesc.setColorWrites(false, false, false, false);
        depthDesc.setZFunc(ComparisonFunction::LESS);
        _depthStateBlockHash = depthDesc.getHash();
    }
    return _depthStateBlockHash;
}

U32 SceneNodeRenderState::getShadowStateBlock() {
    if (_shadowStateBlockHash == 0) {
        RenderStateBlock depthDesc;
        /// Cull back faces for shadow rendering
        depthDesc.setCullMode(CullMode::CCW);
        // depthDesc.setZBias(1.0f, 2.0f);
        depthDesc.setColorWrites(true, true, false, false);
        _shadowStateBlockHash = depthDesc.getHash();
    }
    return _shadowStateBlockHash;
}

bool SceneNodeRenderState::getDrawState(RenderStage currentStage) const {
    return _drawState &&
            !BitCompare(_exclusionMask, to_uint(1 << (to_uint(currentStage) + 1)));
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage stage) {
    _exclusionMask |= (1 << (to_uint(stage) + 1));
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStage stage) {
    _exclusionMask &= ~(1 << (to_uint(stage) + 1));
}

};