#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderStageBuilder.h"
#include "CustomOperators/Headers/SSAOPreRenderOperator.h"
#include "CustomOperators/Headers/BloomPreRenderOperator.h"

PreRenderStageBuilder::PreRenderStageBuilder(){
	_renderStage = New PreRenderStage();
}

PreRenderStageBuilder::~PreRenderStageBuilder(){
	if(_renderStage){
		delete _renderStage;
		_renderStage = NULL;
	}
}

PreRenderOperator*  PreRenderStageBuilder::addSSAOOperator(ShaderProgram* const SSAOShader, Quad3D* target, bool& state, FrameBufferObject* result) {
	SSAOPreRenderOperator* ssao = New SSAOPreRenderOperator(SSAOShader,target,result);
	return addToStage(ssao, state);
}

PreRenderOperator*  PreRenderStageBuilder::addBloomOperator(ShaderProgram* const bloomShader, Quad3D* target, bool& state, FrameBufferObject* result) {
	BloomPreRenderOperator* bloom = New BloomPreRenderOperator(bloomShader,target,result);
	return addToStage(bloom, state);
}

PreRenderOperator* PreRenderStageBuilder::addToStage(PreRenderOperator* op, bool& state){
	op->setEnabled(state);
	_renderStage->addRenderOperator(op);
	return op;
}