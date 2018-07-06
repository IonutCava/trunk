#ifndef _PRE_RENDER_STAGE_BUILDER_H_
#define _PRE_RENDER_STAGE_BUILDER_H_

#include "core.h" 
///inspired by ImageFilters in Pshyce http://www.codinglabs.net/psyche.aspx
///actual implementation and roll differs

class ShaderProgram;
class Quad3D;
class PreRenderStage;
class FrameBufferObject;
class PreRenderOperator;
DEFINE_SINGLETON(PreRenderStageBuilder)

public:
	PreRenderStageBuilder();
   ~PreRenderStageBuilder();
   ///Adding PreRenderOperators to the PreRender stage, needs an input shader to be applied to the scene/target
   ///A target fullscreen quad to which it should render it's output. This output is saved in the "result" FBO
   ///"state" is a reference to the global variable that enables or disables the effect (via options, PostFX, config, etc)
   PreRenderOperator* 	 addGenericOperator(ShaderProgram* const shader, Quad3D* const target, bool& state, FrameBufferObject* result, const vec2<U16>& resolution);
   PreRenderOperator* 	 addSSAOOperator(ShaderProgram* const SSAOShader, Quad3D* const target, bool& state, FrameBufferObject* result, const vec2<U16>& resolution);
   PreRenderOperator*    addBloomOperator(ShaderProgram* const bloomShader, Quad3D* const target, bool& state, FrameBufferObject* result, const vec2<U16>& resolution);
   PreRenderOperator*    addDOFOperator(ShaderProgram* const dofShader, Quad3D* const target, bool& state, FrameBufferObject* result, const vec2<U16>& resolution);
   PreRenderOperator*    addDeferredShadingOperator(ShaderProgram* const deferredShader, Quad3D* const target, bool& state, FrameBufferObject* result, const vec2<U16>& resolution);

   inline PreRenderStage*	getPreRenderBatch() { return _renderStage; };

private:
	PreRenderOperator*  addToStage(PreRenderOperator* op, bool& state);

private:

	PreRenderStage	*_renderStage;

END_SINGLETON

#endif