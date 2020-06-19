#pragma once
#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Platform/Video/Headers/RenderStagePass.h"

namespace Divide {

struct RenderStagePass;

struct SceneNodeRenderState {

    bool drawState(const RenderStagePass& stagePass, U8 LoD) const;
    /// variant = -1 => all variants
    void addToDrawExclusionMask(RenderStage stage, RenderPassType passType = RenderPassType::COUNT, U8 variant = g_AllVariantsID, U16 index = g_AllIndicesID, U16 pass = g_AllPassID);

    PROPERTY_RW(bool, drawState, true);
    PROPERTY_RW(U8, minLodLevel, 255u);

   protected:
    vectorEASTL<RenderStagePass> _exclusionStagePasses;
};

};  // namespace Divide

#endif
