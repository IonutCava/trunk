#ifndef _PRE_RenderStage_BUILDER_H_
#define _PRE_RenderStage_BUILDER_H_

#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DoFPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/FXAAPreRenderOperator.h"
///inspired by ImageFilters in Pshyce http://www.codinglabs.net/psyche.aspx
///actual implementation and roll differ

namespace Divide {

class ShaderProgram;
class Quad3D;
class PreRenderStage;
class Framebuffer;
struct ScreenSampler;
DEFINE_SINGLETON(PreRenderStageBuilder)

public:
	PreRenderStageBuilder();
   ~PreRenderStageBuilder();
   ///Adding PreRenderOperators to the PreRender stage, needs an input shader to be applied to the scene/target
   ///A target fullscreen quad to which it should render it's output. This output is saved in the "result" FB
   ///"state" is a reference to the global variable that enables or disables the effect (via options, PostFX, config, etc)
   template<class T>
   inline PreRenderOperator* addPreRenderOperator(bool& state,
												  Framebuffer* result,
												  const vec2<U16>& resolution){
			return addToStage(New T(result,resolution,_screenSampler),state);
   }

   inline PreRenderStage*	getPreRenderBatch() { return _renderStage; };

private:
	PreRenderOperator*  addToStage(PreRenderOperator* op, bool& state);

private:

	PreRenderStage*    _renderStage;
	SamplerDescriptor* _screenSampler;
END_SINGLETON

}; //namespace Divide
#endif