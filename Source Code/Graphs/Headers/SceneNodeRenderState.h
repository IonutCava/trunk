#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

struct RenderStagePass;
enum class RenderStage : U32;

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

    size_t getDepthStateBlock();
    size_t getShadowStateBlock();

   protected:
    bool _drawState;
    bool _noDefaultMaterial;
    vectorImpl<U32> _exclusionMask;

    size_t _depthStateBlockHash;
    size_t _shadowStateBlockHash;
};

};  // namespace Divide

#endif
