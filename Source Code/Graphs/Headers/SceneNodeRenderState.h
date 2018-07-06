#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "Hardware/Video/Headers/RenderStateBlock.h"

class SceneNodeRenderState {
public:
    SceneNodeRenderState() : _drawState(true),
                             _noDefaultMaterial(false),
                             _exclusionMask(0),
                             _depthStateBlockHash(0),
                             _shadowStateBlockHash(0),
                             _isVisible(true),
                             _hasWaterReflection(true)
    {
    }

    ~SceneNodeRenderState();

    inline  void useDefaultMaterial(bool state) {_noDefaultMaterial = !state;}
    inline  void setDrawState(bool state) {_drawState = state;}
            bool getDrawState()  const {return _drawState;}
            bool getDrawState(const RenderStage& currentStage)  const;
            void addToDrawExclusionMask(U32 stageMask);
            void removeFromDrawExclusionMask(U32 stageMask);
            I64 getDepthStateBlock();
            I64 getShadowStateBlock();

protected:
    friend class SceneNode;
    bool _hasWaterReflection;
    bool _isVisible;
    bool _drawState;
    bool _noDefaultMaterial;
    U32  _exclusionMask;

    I64 _depthStateBlockHash;
    I64 _shadowStateBlockHash;
};

#endif