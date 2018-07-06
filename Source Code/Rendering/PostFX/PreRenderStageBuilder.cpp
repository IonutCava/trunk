#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderStageBuilder.h"

PreRenderStageBuilder::PreRenderStageBuilder(){
	_renderStage = New PreRenderStage();
}

PreRenderStageBuilder::~PreRenderStageBuilder(){

	SAFE_DELETE(_renderStage);
}

PreRenderOperator* PreRenderStageBuilder::addToStage(PreRenderOperator* op, bool& state){
	op->setEnabled(state);
	_renderStage->addRenderOperator(op);
	return op;
}