#ifndef _PRE_RenderStage_BUILDER_H_
#define _PRE_RenderStage_BUILDER_H_

#include "Core/Headers/Singleton.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DoFPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/FXAAPreRenderOperator.h"
///inspired by ImageFilters in Pshyce http://www.codinglabs.net/psyche.aspx
///actual implementation and roll differ

class ShaderProgram;
class Quad3D;
class PreRenderStage;
class FrameBufferObject;

DEFINE_SINGLETON(PreRenderStageBuilder)

public:
	PreRenderStageBuilder();
   ~PreRenderStageBuilder();
   ///Adding PreRenderOperators to the PreRender stage, needs an input shader to be applied to the scene/target
   ///A target fullscreen quad to which it should render it's output. This output is saved in the "result" FBO
   ///"state" is a reference to the global variable that enables or disables the effect (via options, PostFX, config, etc)
   template<class T>
   inline PreRenderOperator* addPreRenderOperator(Quad3D* const target,
												  bool& state,
												  FrameBufferObject* result,
												  const vec2<U16>& resolution){
			return addToStage(New T(target,result,resolution),state);
   }

   inline PreRenderStage*	getPreRenderBatch() { return _renderStage; };

private:
	PreRenderOperator*  addToStage(PreRenderOperator* op, bool& state);

private:

	PreRenderStage	*_renderStage;

END_SINGLETON

#endif