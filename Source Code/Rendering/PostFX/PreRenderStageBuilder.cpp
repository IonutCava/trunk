#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderStageBuilder.h"

#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/Textures/Headers/TextureDescriptor.h"

PreRenderStageBuilder::PreRenderStageBuilder(){
	_renderStage = New PreRenderStage();
	_screenSampler = New SamplerDescriptor;
	_screenSampler->setWrapMode(TEXTURE_CLAMP_TO_EDGE);
	_screenSampler->toggleMipMaps(false); //it's a flat texture on a full screen quad. really?
}

PreRenderStageBuilder::~PreRenderStageBuilder(){
	SAFE_DELETE(_renderStage);
	SAFE_DELETE(_screenSampler);
}

PreRenderOperator* PreRenderStageBuilder::addToStage(PreRenderOperator* op, bool& state){
	op->enabled(state);
	_renderStage->addRenderOperator(op);
	return op;
}