#include "stdafx.h"

#include "Headers/SceneNodeRenderState.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

bool SceneNodeRenderState::drawState(const RenderStagePass& stagePass, const U8 LoD) const {
    if (!_drawState || LoD >= minLodLevel()) {
        return false;
    }

    for (const RenderStagePass& exclussionStagePass : _exclusionStagePasses) {
        if ((exclussionStagePass._variant == stagePass._variant) &&
            (exclussionStagePass._passIndex == g_AllPassIndexesID || exclussionStagePass._passIndex == stagePass._passIndex) &&
            (exclussionStagePass._stage == RenderStage::COUNT || exclussionStagePass._stage == stagePass._stage) &&
            (exclussionStagePass._passType == RenderPassType::COUNT || exclussionStagePass._passType == stagePass._passType))
        {
            return false;
        }
    }

    return true;
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage stage, RenderPassType passType, I16 variant, U32 passIndex) {
    if (variant <= -1) {
        for (U8 i = 0; i < Material::g_maxVariantsPerPass; ++i) {
            addToDrawExclusionMask(stage, passType, to_I16(i), passIndex);
        }
    } else {
        assert(variant < Material::g_maxVariantsPerPass);
        
        RenderStagePass stagePass{ stage, passType, to_U8(variant), passIndex};
        if (std::find(std::cbegin(_exclusionStagePasses), std::cend(_exclusionStagePasses), stagePass) == std::cend(_exclusionStagePasses)) {
            _exclusionStagePasses.emplace_back(stagePass);
        }
    }
    
}

void SceneNodeRenderState::removeFromDrawExclusionMask(RenderStage stage, RenderPassType passType, I64 variant, U32 passIndex) {
    if (variant <= -1) {
        for (U8 i = 0; i < Material::g_maxVariantsPerPass; ++i) {
            removeFromDrawExclusionMask(stage, passType, to_I16(i), passIndex);
        }
    } else {
        assert(variant < Material::g_maxVariantsPerPass);

        RenderStagePass stagePass{ stage, passType, to_U8(variant), passIndex};
        
        auto it = std::find(std::cbegin(_exclusionStagePasses), std::cend(_exclusionStagePasses), stagePass);
        if (it != std::cend(_exclusionStagePasses)) {
            _exclusionStagePasses.erase(it);
        } else {
            // Maybe handle this better? If we added a mask using the "g_AllPassIndexes" but want to remove a single specific pass, add all of the others?
            // IDK, too complicated ... Just remove the general one for now and cross that bridge when we get there
            removeFromDrawExclusionMask(stage, passType, variant, g_AllPassIndexesID);
        }
    }
}

};