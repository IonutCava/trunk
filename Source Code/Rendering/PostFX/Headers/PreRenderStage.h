#ifndef _PRE_RENDER_STAGE_H_
#define _PRE_RENDER_STAGE_H_

#include "core.h"

//PreRenderStage is a class specialized in applying certain shaders and generating FBO results
//
class PreRenderOperator;
class PreRenderStage{
public:
	PreRenderStage(){}
	~PreRenderStage();
	bool addRenderOperator(PreRenderOperator* op) {_operators.push_back(op); return true;}
	void execute();
	void reshape(I32 width, I32 height);

private:
	std::vector<PreRenderOperator* >	_operators;
	bool								_isValid;		//True if the filter is valid
};

#endif