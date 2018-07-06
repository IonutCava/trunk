#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderOperator.h"

namespace Divide {

PreRenderStage::~PreRenderStage(){
	for(PreRenderOperator*& op : _operators){
		SAFE_DELETE(op);
	}
}

void PreRenderStage::execute(){
	for(PreRenderOperator* op : _operators){
		op->operation();
	}
}

void PreRenderStage::reshape(I32 width, I32 height){
	for(PreRenderOperator* op : _operators){
		op->reshape(width,height);
	}
}

};