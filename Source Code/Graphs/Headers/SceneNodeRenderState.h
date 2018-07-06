#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

struct RenderStagePass;
enum class RenderStage : U8;

class SceneNodeRenderState {
   public:
    SceneNodeRenderState();
    ~SceneNodeRenderState();

    inline void setDrawState(bool state) { _drawState = state; }
    inline bool getDrawState() const { return _drawState; }

    bool getDrawState(RenderStagePass currentStagePass) const;

    void addToDrawExclusionMask(RenderStagePass currentStagePass);
    void removeFromDrawExclusionMask(RenderStagePass currentStagePass);

    void addToDrawExclusionMask(RenderStage currentStage);
    void removeFromDrawExclusionMask(RenderStage currentStage);

   protected:
    bool _drawState;
    vector<U32> _exclusionMask;
};

};  // namespace Divide

#endif
