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

    const auto checkIndex = [&stagePass](const RenderStagePass& exclusion) noexcept {
        const bool mainIndexMatch = exclusion._passIndex == g_AllPassIndexesID || //All Passes
                                    exclusion._passIndex == stagePass._passIndex;//Same pass

        const bool subIndexMatch = (exclusion._indexA == stagePass._indexA && //Sub pass index 1 match
                                         (exclusion._indexB == g_AllPassSubIndexID ||
                                          exclusion._indexB == stagePass._indexB)); //Sub pass index 2 match or all index 2 sub pass indices

        return mainIndexMatch || subIndexMatch;
    };

    for (const RenderStagePass& exclussionStagePass : _exclusionStagePasses) {
        if (checkIndex(exclussionStagePass) && 
            (exclussionStagePass._variant == stagePass._variant) &&
            (exclussionStagePass._stage == RenderStage::COUNT || exclussionStagePass._stage == stagePass._stage) &&
            (exclussionStagePass._passType == RenderPassType::COUNT || exclussionStagePass._passType == stagePass._passType))
        {
            return false;
        }
    }

    return true;
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage stage, RenderPassType passType, I16 variant, U16 indexA, U16 indexB) {
    if (variant <= -1) {
        for (U8 i = 0; i < Material::g_maxVariantsPerPass; ++i) {
            addToDrawExclusionMask(stage, passType, to_I16(i), indexA, indexB);
        }
    } else {
        assert(variant < Material::g_maxVariantsPerPass);

        RenderStagePass stagePass{ stage, passType, to_U8(variant) };
        stagePass._indexA = indexA;
        stagePass._indexA = indexB;

        if (eastl::find(eastl::cbegin(_exclusionStagePasses), eastl::cend(_exclusionStagePasses), stagePass) == eastl::cend(_exclusionStagePasses)) {
            _exclusionStagePasses.emplace_back(stagePass);
        }
    }
}

void SceneNodeRenderState::addToDrawExclusionMask(RenderStage stage, RenderPassType passType, I16 variant, U32 passIndex) {
    if (variant <= -1) {
        for (U8 i = 0; i < Material::g_maxVariantsPerPass; ++i) {
            addToDrawExclusionMask(stage, passType, to_I16(i), passIndex);
        }
    } else {
        assert(variant < Material::g_maxVariantsPerPass);
        
        RenderStagePass stagePass{ stage, passType, to_U8(variant), passIndex};
        if (eastl::find(eastl::cbegin(_exclusionStagePasses), eastl::cend(_exclusionStagePasses), stagePass) == eastl::cend(_exclusionStagePasses)) {
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
        
        auto it = eastl::find(eastl::cbegin(_exclusionStagePasses), eastl::cend(_exclusionStagePasses), stagePass);
        if (it != eastl::cend(_exclusionStagePasses)) {
            _exclusionStagePasses.erase(it);
        } else {
            // Maybe handle this better? If we added a mask using the "g_AllPassIndexes" but want to remove a single specific pass, add all of the others?
            // IDK, too complicated ... Just remove the general one for now and cross that bridge when we get there
            removeFromDrawExclusionMask(stage, passType, variant, g_AllPassIndexesID);
        }
    }
}

};