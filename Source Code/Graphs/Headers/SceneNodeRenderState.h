#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#define NULL 0
#include "Hardware/Video/Headers/RenderStateBlock.h"

class SceneNodeRenderState {
public:
    SceneNodeRenderState() : _drawState(true),
                             _noDefaultMaterial(false),
                             _exclusionMask(0),
                             _depthStateBlock(NULL),
                             _shadowStateBlock(NULL),
                             _isVisible(true),
                             _hasWaterReflection(true)
    {
    }

    ~SceneNodeRenderState();

    inline  void useDefaultMaterial(bool state) {_noDefaultMaterial = !state;}
    inline  void setDrawState(bool state) {_drawState = state;}
            bool getDrawState()  const {return _drawState;}
            bool getDrawState(const RenderStage& currentStage)  const;
            void addToDrawExclusionMask(I32 stageMask);
            void removeFromDrawExclusionMask(I32 stageMask);
            RenderStateBlock* getDepthStateBlock();
            RenderStateBlock* getShadowStateBlock();
protected:
    friend class SceneNode;
    bool _hasWaterReflection;
    bool _isVisible;
    bool _drawState;
    bool _noDefaultMaterial;
    U16  _exclusionMask;

    RenderStateBlock* _depthStateBlock;
    RenderStateBlock* _shadowStateBlock;
};

#endif