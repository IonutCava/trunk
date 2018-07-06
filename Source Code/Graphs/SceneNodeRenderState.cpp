#include "Headers/SceneNodeRenderState.h"
#include "Hardware/Video/Headers/GFXDevice.h"

RenderStateBlock* SceneNodeRenderState::getDepthStateBlock(){
	if(!_depthStateBlock){
		RenderStateBlockDescriptor depthDesc;
		/// Cull back faces for shadow rendering
		depthDesc.setCullMode(CULL_MODE_CCW);
		depthDesc._fixedLighting = false;
		depthDesc._zBias = 1.0f;
		depthDesc.setColorWrites(false,false,false,true);
		_depthStateBlock = GFX_DEVICE.createStateBlock(depthDesc);
	}
	return _depthStateBlock;
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
	return !bitCompare(_exclusionMask,currentStage);
}