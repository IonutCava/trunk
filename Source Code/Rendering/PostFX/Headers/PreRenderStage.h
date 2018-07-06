#ifndef _PRE_RenderStage_H_
#define _PRE_RenderStage_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"

namespace Divide {

//PreRenderStage is a class specialized in applying certain shaders and generating FB results
//
class PreRenderOperator;
class PreRenderStage{
public:
	PreRenderStage(){}
	~PreRenderStage();
	inline bool addRenderOperator(PreRenderOperator* op) {
        _operators.push_back(op); 
        return true;
    }
	void execute();
	void reshape(I32 width, I32 height);

private:
	vectorImpl<PreRenderOperator* >	_operators;
	bool							_isValid;		//True if the filter is valid
};

}; //namespace Divide
#endif