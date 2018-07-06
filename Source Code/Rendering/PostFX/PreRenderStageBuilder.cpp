#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderStageBuilder.h"
#include "CustomOperators/Headers/SSAOPreRenderOperator.h"
#include "CustomOperators/Headers/BloomPreRenderOperator.h"

PreRenderStageBuilder::PreRenderStageBuilder(){
	_renderStage = New PreRenderStage();
}

PreRenderStageBuilder::~PreRenderStageBuilder(){

	SAFE_DELETE(_renderStage);
}

PreRenderOperator*  PreRenderStageBuilder::addSSAOOperator(ShaderProgram* const SSAOShader, Quad3D* target, bool& state, FrameBufferObject* result, const vec2<U16>& resolution) {
	SSAOPreRenderOperator* ssao = New SSAOPreRenderOperator(SSAOShader,target,result,resolution);
	return addToStage(ssao, state);
}

PreRenderOperator*  PreRenderStageBuilder::addBloomOperator(ShaderProgram* const bloomShader, Quad3D* target, bool& state, FrameBufferObject* result, const vec2<U16>& resolution) {
	BloomPreRenderOperator* bloom = New BloomPreRenderOperator(bloomShader,target,result,resolution);
	return addToStage(bloom, state);
}

PreRenderOperator* PreRenderStageBuilder::addToStage(PreRenderOperator* op, bool& state){
	op->setEnabled(state);
	_renderStage->addRenderOperator(op);
	return op;
}