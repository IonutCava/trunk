#pragma once
#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Platform/Video/Headers/RenderStagePass.h"

namespace Divide {

struct RenderStagePass;

struct SceneNodeRenderState {

    static const  U32 g_AllPassIndexesID = std::numeric_limits<U32>::max();
    static const  U32 g_AllPassSubIndexID = std::numeric_limits<U16>::max();

    bool drawState(const RenderStagePass& stagePass, const U8 LoD) const;

    /// variant = -1 => all variants
    void addToDrawExclusionMask(RenderStage stage, RenderPassType passType, I16 variant, U32 passIndex = g_AllPassIndexesID);
    void addToDrawExclusionMask(RenderStage stage, RenderPassType passType, I16 variant, U16 indexA, U16 indexB = g_AllPassSubIndexID);
    void removeFromDrawExclusionMask(RenderStage stage, RenderPassType passType, I64 variant, U32 passIndex = g_AllPassIndexesID);

    PROPERTY_RW(bool, drawState, true);
    PROPERTY_RW(U8, minLodLevel, 255u);

   protected:
    vectorEASTL<RenderStagePass> _exclusionStagePasses;
};

};  // namespace Divide

#endif
