#include "Headers/SceneNodeRenderState.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

SceneNodeRenderState::SceneNodeRenderState()
  : _drawState(true),
    _noDefaultMaterial(false),
    _depthStateBlockHash(0),
    _shadowStateBlockHash(0),
    _exclusionMask(to_const_U32(RenderPassType::COUNT), 0u)
{
}

SceneNodeRenderState::~SceneNodeRenderState()
{
}

size_t SceneNodeRenderState::getDepthStateBlock() {
    if (_depthStateBlockHash == 0) {
        RenderStateBlock depthDesc;
        depthDesc.setColourWrites(false, false, false, false);
        depthDesc.setZFunc(ComparisonFunction::LESS);
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
        depthDesc.setColourWrites(true, true, false, false);
        _shadowStateBlockHash = depthDesc.getHash();
    }
    return _shadowStateBlockHash;
}

bool SceneNodeRenderState::getDrawState(const RenderStagePass& currentStagePass) const {
    return _drawState &&
           !BitCompare(_exclusionMask[to_U32(currentStagePass._passType)], to_U32(1 << (to_U32(currentStagePass._stage) + 1)));
}

void SceneNodeRenderState::addToDrawExclusionMask(const RenderStagePass& currentStagePass) {
    _exclusionMask[to_U32(currentStagePass._passType)] |= (1 << (to_U32(currentStagePass._stage) + 1));
}

void SceneNodeRenderState::removeFromDrawExclusionMask(const RenderStagePass& currentStagePass) {
    _exclusionMask[to_U32(currentStagePass._passType)] &= ~(1 << (to_U32(currentStagePass._stage) + 1));
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage currentStage) {
    for (U8 pass = 0; pass < to_const_U32(RenderPassType::COUNT); ++pass) {
        addToDrawExclusionMask(RenderStagePass(currentStage, static_cast<RenderPassType>(pass)));
    }
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStage currentStage) {
    for (U8 pass = 0; pass < to_const_U32(RenderPassType::COUNT); ++pass) {
        removeFromDrawExclusionMask(RenderStagePass(currentStage, static_cast<RenderPassType>(pass)));
    }
}

};