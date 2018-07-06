#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class RenderStagePass;
enum class RenderStage : U8;

class SceneNodeRenderState {
   public:
    SceneNodeRenderState();
    ~SceneNodeRenderState();

    inline void useDefaultMaterial(bool state) { _noDefaultMaterial = !state; }
    inline bool useDefaultMaterial() const { return !_noDefaultMaterial; }
    inline void setDrawState(bool state) { _drawState = state; }
    inline bool getDrawState() const { return _drawState; }

    bool getDrawState(const RenderStagePass& currentStagePass) const;

    void addToDrawExclusionMask(const RenderStagePass& currentStagePass);
    void removeFromDrawExclusionMask(const RenderStagePass& currentStagePass);

    void addToDrawExclusionMask(RenderStage currentStage);
    void removeFromDrawExclusionMask(RenderStage currentStage);

   protected:
    bool _drawState;
    bool _noDefaultMaterial;
    vector<U32> _exclusionMask;
};

};  // namespace Divide

#endif
