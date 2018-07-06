#include "Headers/SceneNodeRenderState.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

SceneNodeRenderState::~SceneNodeRenderState() {}

size_t SceneNodeRenderState::getDepthStateBlock() {
    if (_depthStateBlockHash == 0) {
        RenderStateBlock depthDesc;
        depthDesc.setColorWrites(false, false, false, false);
        _depthStateBlockHash = depthDesc.getHash();
    }
    return _depthStateBlockHash;
}

size_t SceneNodeRenderState::getShadowStateBlock() {
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
            !BitCompare(_exclusionMask,
                        (to_bitwise(to_uint(currentStage) + 1)));
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage stage) {
    _exclusionMask |= (to_bitwise(to_uint(stage) + 1));
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStage stage) {
    _exclusionMask &= ~(to_bitwise(to_uint(stage) + 1));
}

};