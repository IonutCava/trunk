#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderOperator.h"

PreRenderStage::~PreRenderStage(){
	FOR_EACH(PreRenderOperator* op, _operators){
		SAFE_DELETE(op);
	}
}

void PreRenderStage::execute(){
	FOR_EACH(PreRenderOperator* op, _operators){
		op->operation();
	}
}

void PreRenderStage::reshape(I32 width, I32 height){
	FOR_EACH(PreRenderOperator* op, _operators){
		op->reshape(width,height);
	}
}