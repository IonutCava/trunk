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

bool SceneNodeRenderState::getDrawState(RenderStagePass currentStagePass) const {
    return _drawState &&
           !BitCompare(_exclusionMask[to_U32(currentStagePass._passType)], to_U32(1 << (to_U32(currentStagePass._stage) + 1)));
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStagePass currentStagePass) {
    _exclusionMask[to_U32(currentStagePass._passType)] |= (1 << (to_U32(currentStagePass._stage) + 1));
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStagePass currentStagePass) {
    _exclusionMask[to_U32(currentStagePass._passType)] &= ~(1 << (to_U32(currentStagePass._stage) + 1));
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage currentStage) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        addToDrawExclusionMask(RenderStagePass(currentStage, static_cast<RenderPassType>(pass)));
    }
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStage currentStage) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        removeFromDrawExclusionMask(RenderStagePass(currentStage, static_cast<RenderPassType>(pass)));
    }
}

};