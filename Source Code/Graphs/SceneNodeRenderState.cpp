#include "stdafx.h"

#include "Headers/SceneNodeRenderState.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

bool SceneNodeRenderState::getDrawState(const RenderStagePass& stagePass) const {
    if (!_drawState || _exclusionStage == stagePass._stage || _exclusionPassType == stagePass._passType) {
        return false;
    }

    for (const RenderStagePass& it : _exclusionStagePasses) {
        if (it == stagePass) {
            return false;
        }
    }

    return true;
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStagePass stagePass) {
    if (std::find(std::cbegin(_exclusionStagePasses), std::cend(_exclusionStagePasses), stagePass) == std::cend(_exclusionStagePasses)) {
        _exclusionStagePasses.push_back(stagePass);
    }
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStagePass stagePass) {
    auto it = std::find(std::cbegin(_exclusionStagePasses), std::cend(_exclusionStagePasses), stagePass);
    if (it != std::cend(_exclusionStagePasses)) {
        _exclusionStagePasses.erase(it);
    }
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage stage) {
    _exclusionStage = stage;
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStage stage) {
    _exclusionStage = RenderStage::COUNT;
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderPassType passType) {
    _exclusionPassType = passType;
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderPassType passType) {
    _exclusionPassType = RenderPassType::COUNT;
}

};