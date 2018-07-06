#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderOperator.h"

PreRenderStage::~PreRenderStage(){
	for_each(PreRenderOperator* op, _operators){
		SAFE_DELETE(op);
	}
}

void PreRenderStage::execute(){
	for_each(PreRenderOperator* op, _operators){
		op->operation();
	}
}

void PreRenderStage::reshape(I32 width, I32 height){
	for_each(PreRenderOperator* op, _operators){
		op->reshape(width,height);
	}
}