#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Platform/Video/Headers/RenderStagePass.h"

namespace Divide {

struct RenderStagePass;

class SceneNodeRenderState {
   public:
    bool drawState(const RenderStagePass& stagePass, const U8 LoD) const;

    void addToDrawExclusionMask(RenderStagePass stagePass);
    void removeFromDrawExclusionMask(RenderStagePass stagePass);

    void addToDrawExclusionMask(RenderStage stage);
    void removeFromDrawExclusionMask(RenderStage stage);

    void addToDrawExclusionMask(RenderPassType passType);
    void removeFromDrawExclusionMask(RenderPassType passType);

    PROPERTY_RW(bool, drawState, true);
    PROPERTY_RW(U8, minLodLevel, 255u);

   protected:
    RenderStage _exclusionStage = RenderStage::COUNT;
    RenderPassType _exclusionPassType = RenderPassType::COUNT;
    vector<RenderStagePass> _exclusionStagePasses;
};

};  // namespace Divide

#endif
