#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

struct RenderStagePass;
enum class RenderStage : U8;
enum class RenderPassType : U8;

class SceneNodeRenderState {
   public:
    SceneNodeRenderState();
    ~SceneNodeRenderState();

    inline void setDrawState(bool state) { _drawState = state; }
    inline bool getDrawState() const { return _drawState; }

    bool getDrawState(RenderStagePass stagePass) const;

    void addToDrawExclusionMask(RenderStagePass stagePass);
    void removeFromDrawExclusionMask(RenderStagePass stagePass);

    void addToDrawExclusionMask(RenderStage stage);
    void removeFromDrawExclusionMask(RenderStage stage);

    void addToDrawExclusionMask(RenderPassType passType);
    void removeFromDrawExclusionMask(RenderPassType passType);

   protected:
    bool _drawState;
    vector<U32> _exclusionMask;
};

};  // namespace Divide

#endif
