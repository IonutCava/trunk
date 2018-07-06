#ifndef _PRE_RENDER_OPERATOR_H_
#define _PRE_RENDER_OPERATOR_H_

#include "Utility/Headers/Vector.h"
#include "Core/Math/Headers/MathClasses.h"

enum RenderStage;
class Quad3D;
class FrameBufferObject;
class SamplerDescriptor;
///It's called a prerender operator because it operates on the buffer before "rendering" to the screen
///Technically, it's a post render operation
class PreRenderOperator {

protected:
	enum PostFXRenderStage {
		FXAA_STAGE        = toBit(1),
	    SSAO_STAGE	      = toBit(2),
	    DOF_STAGE         = toBit(3),
		BLOOM_STAGE       = toBit(4),
		LIGHT_SHAFT_STAGE = toBit(5)
	};

public:
	///The RenderStage is used to inform the GFXDevice of what we are currently doing to set up apropriate states
	///The target is the full screen quad to which we want to apply our operation to generate the result
	PreRenderOperator(PostFXRenderStage stage, Quad3D* target,
		              const vec2<U16>& resolution, SamplerDescriptor* const sampler) : _internalSampler(sampler),
					                                                                   _stage(stage),
																					   _renderQuad(target),
																					   _resolution(resolution),
																					   _genericFlag(false)
	{
	}

	virtual ~PreRenderOperator()
	{
	}

	virtual void operation() = 0;
	virtual void reshape(I32 width, I32 height) = 0;
	///Reference to state
	inline void enabled(bool state)           {_enabled = state;}
	inline bool enabled()               const {return _enabled; }
	inline void genericFlag(bool state)       {_genericFlag = state;}
	inline bool genericFlag()           const {return _genericFlag;}
	inline void addInputFBO(FrameBufferObject* const input)          {_inputFBO.push_back(input);}

protected:

	///Target to render to;
	Quad3D*	_renderQuad;
	bool    _enabled;
	///Used to represent anything (for example: HDR on/off for bloom)
	bool    _genericFlag;
	vectorImpl<FrameBufferObject* > _inputFBO;
	vec2<U16> _resolution;
	SamplerDescriptor* _internalSampler;

private:
	PostFXRenderStage _stage;
};

#endif