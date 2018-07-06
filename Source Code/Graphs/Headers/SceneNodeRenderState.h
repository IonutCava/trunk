#ifndef _SCENE_NODE_RENDER_STATE_H_
#define _SCENE_NODE_RENDER_STATE_H_

#include "core.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

class SceneNodeRenderState {
public:
	SceneNodeRenderState() : _drawState(true),
							 _noDefaultMaterial(false),
							 _exclusionMask(0),
							 _depthStateBlock(NULL),
							 _isVisible(true),
							 _hasWaterReflection(true)
	{
	}

	~SceneNodeRenderState()
	{
		SAFE_DELETE(_depthStateBlock);
	}

	inline  void useDefaultMaterial(bool state) {_noDefaultMaterial = !state;}
	inline  void setDrawState(bool state) {_drawState = state;}
	     	bool getDrawState()  const {return _drawState;} 
			bool getDrawState(RenderStage currentStage)  const;
			void addToDrawExclusionMask(I32 stageMask);
			void removeFromDrawExclusionMask(I32 stageMask);
			RenderStateBlock* getDepthStateBlock();
protected:
	friend class SceneNode;
	bool _hasWaterReflection;
	bool _isVisible;
	bool _drawState;
	bool _noDefaultMaterial;
	U8  _exclusionMask;

	RenderStateBlock* _depthStateBlock;
};

#endif