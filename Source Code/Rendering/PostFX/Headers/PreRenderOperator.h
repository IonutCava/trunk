#ifndef _PRE_RENDER_OPERATOR_H_
#define _PRE_RENDER_OPERATOR_H_

#include "core.h"

enum RENDER_STAGE;
class Quad3D;
class FrameBufferObject;
class PreRenderOperator {
public:
	///The RENDER_STAGE is used to inform the GFXDevice of what we are currently doing to set up apropriate states
	///The target is the full screen quad to which we want to apply our operation to generate the result
	PreRenderOperator(RENDER_STAGE stage, Quad3D* target, const vec2<U16>& resolution) : _stage(stage), _renderQuad(target), _resolution(resolution) {};
	virtual ~PreRenderOperator() {};

	virtual void operation() = 0;
	virtual void reshape(I32 width, I32 height) = 0;
	///Reference to state
	inline void setEnabled(bool& state) {_enabled = state;}
	inline bool getEnabled()            {return _enabled; }
	
	inline void addInputFBO(FrameBufferObject* input) {_inputFBO.push_back(input);}

protected:
	///Target to render to;
	Quad3D*	_renderQuad;
	bool _enabled;
	std::vector<FrameBufferObject* > _inputFBO;
	vec2<U16> _resolution;

private:
	RENDER_STAGE _stage;

};

#endif