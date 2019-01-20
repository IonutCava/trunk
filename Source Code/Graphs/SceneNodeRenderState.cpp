#include "stdafx.h"

#include "Headers/SceneNodeRenderState.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

SceneNodeRenderState::SceneNodeRenderState()
  : _drawState(true),
    _exclusionMask(to_base(RenderPassType::COUNT), 0u)
{
}

SceneNodeRenderState::~SceneNodeRenderState()
{
}

bool SceneNodeRenderState::getDrawState(RenderStagePass stagePass) const {
    return _drawState &&
           !BitCompare(_exclusionMask[to_U32(stagePass._passType)], to_U32(1 << (to_U32(stagePass._stage) + 1)));
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStagePass stagePass) {
    _exclusionMask[to_U32(stagePass._passType)] |= (1 << (to_U32(stagePass._stage) + 1));
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStagePass stagePass) {
    _exclusionMask[to_U32(stagePass._passType)] &= ~(1 << (to_U32(stagePass._stage) + 1));
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage stage) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        addToDrawExclusionMask(RenderStagePass(stage, static_cast<RenderPassType>(pass)));
    }
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStage stage) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        removeFromDrawExclusionMask(RenderStagePass(stage, static_cast<RenderPassType>(pass)));
    }
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderPassType passType) {
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        addToDrawExclusionMask(RenderStagePass(static_cast<RenderStage>(stage), passType));
    }
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderPassType passType) {
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        removeFromDrawExclusionMask(RenderStagePass(static_cast<RenderStage>(stage), passType));
    }
}

};