#pragma once
#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Platform/Video/Headers/RenderStagePass.h"

namespace Divide {

struct RenderStagePass;

struct SceneNodeRenderState {

    static constexpr U8  g_AllVariantsID = std::numeric_limits<U8>::max();
    static constexpr U16 g_AllPassID = std::numeric_limits<U16>::max();
    static constexpr U16 g_AllIndiciesID = g_AllPassID;

    bool drawState(const RenderStagePass& stagePass, const U8 LoD) const;
    /// variant = -1 => all variants
    void addToDrawExclusionMask(RenderStage stage, RenderPassType passType = RenderPassType::COUNT, U8 variant = g_AllVariantsID, U16 index = g_AllIndiciesID, U16 pass = g_AllPassID);

    PROPERTY_RW(bool, drawState, true);
    PROPERTY_RW(U8, minLodLevel, 255u);

   protected:
    vectorEASTL<RenderStagePass> _exclusionStagePasses;
};

};  // namespace Divide

#endif
