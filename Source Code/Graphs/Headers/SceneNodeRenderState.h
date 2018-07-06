#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

class SceneNodeRenderState {
   public:
    SceneNodeRenderState()
        : _hasWaterReflection(true),
          _drawState(true),
          _noDefaultMaterial(false),
          _exclusionMask(0),
          _depthStateBlockHash(0),
          _shadowStateBlockHash(0)
    {
    }

    ~SceneNodeRenderState();

    inline void useDefaultMaterial(bool state) { _noDefaultMaterial = !state; }
    inline bool useDefaultMaterial() const { return !_noDefaultMaterial; }
    inline void setDrawState(bool state) { _drawState = state; }
    inline bool getDrawState() const { return _drawState; }

    bool getDrawState(RenderStage currentStage) const;
    void addToDrawExclusionMask(RenderStage stage);
    void removeFromDrawExclusionMask(RenderStage stage);

    U32 getDepthStateBlock();
    U32 getShadowStateBlock();

   protected:
    bool _hasWaterReflection;
    bool _drawState;
    bool _noDefaultMaterial;
    U32 _exclusionMask;

    U32 _depthStateBlockHash;
    U32 _shadowStateBlockHash;
};

};  // namespace Divide

#endif
