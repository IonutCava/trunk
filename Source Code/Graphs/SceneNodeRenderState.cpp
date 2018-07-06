#include "Headers/SceneNodeRenderState.h"
#include "Hardware/Video/Headers/GFXDevice.h"

RenderStateBlock* SceneNodeRenderState::getShadowStateBlock(){
	if(!_shadowStateBlock){
		RenderStateBlockDescriptor shadowDesc;
		/// Cull back faces for shadow rendering
		shadowDesc.setCullMode(CULL_MODE_CCW);
		shadowDesc._fixedLighting = false;
		shadowDesc._zBias = 1.0f;
		shadowDesc.setColorWrites(false,false,false,true);
		_shadowStateBlock = GFX_DEVICE.createStateBlock(shadowDesc);
	}
	return _shadowStateBlock;
}

void SceneNodeRenderState::removeFromDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask &= ~stageMask;
}

void SceneNodeRenderState::addToDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask |= static_cast<RenderStage>(stageMask);
}

bool SceneNodeRenderState::getDrawState(RenderStage currentStage)  const {
	return (_exclusionMask & currentStage) == currentStage ? false : true;
}