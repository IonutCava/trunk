#ifndef _PRE_RENDER_OPERATOR_H_
#define _PRE_RENDER_OPERATOR_H_

#include "Utility/Headers/Vector.h"
#include "Core/Math/Headers/MathClasses.h"

enum RenderStage;
class Quad3D;
class FrameBufferObject;
///It's called a prerender operator because it operates on the buffer before "rendering" to the screen
///Technically, it's a post render operation
class PreRenderOperator {
public:
	///The RenderStage is used to inform the GFXDevice of what we are currently doing to set up apropriate states
	///The target is the full screen quad to which we want to apply our operation to generate the result
	PreRenderOperator(RenderStage stage, Quad3D* target, const vec2<U16>& resolution) : _stage(stage), _renderQuad(target), _resolution(resolution) {};
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
	vectorImpl<FrameBufferObject* > _inputFBO;
	vec2<U16> _resolution;

private:
	RenderStage _stage;
};

#endif