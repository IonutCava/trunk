#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

class SceneNodeRenderState {
   public:
    SceneNodeRenderState()
        : _drawState(true),
          _noDefaultMaterial(false),
          _exclusionMask(0),
          _depthStateBlockHash(0),
          _shadowStateBlockHash(0),
          _isVisible(true),
          _hasWaterReflection(true) {}

    ~SceneNodeRenderState();

    inline void useDefaultMaterial(bool state) { _noDefaultMaterial = !state; }
    inline bool useDefaultMaterial() const { return !_noDefaultMaterial; }
    inline void setDrawState(bool state) { _drawState = state; }
    inline bool getDrawState() const { return _drawState; }

    bool getDrawState(RenderStage currentStage) const;
    void addToDrawExclusionMask(RenderStage stage);
    void removeFromDrawExclusionMask(RenderStage stage);

    size_t getDepthStateBlock();
    size_t getShadowStateBlock();

   protected:
    bool _hasWaterReflection;
    bool _isVisible;
    bool _drawState;
    bool _noDefaultMaterial;
    U32 _exclusionMask;

    size_t _depthStateBlockHash;
    size_t _shadowStateBlockHash;
};

};  // namespace Divide

#endif
