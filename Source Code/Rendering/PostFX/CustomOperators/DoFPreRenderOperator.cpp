#include "Headers/DoFPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h" 
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

DoFPreRenderOperator::DoFPreRenderOperator(ShaderProgram* const DoFShader, 
										   Quad3D* target, 
										   FrameBufferObject* result, 
										   const vec2<U16>& resolution) : PreRenderOperator(DOF_STAGE,target,resolution),
																		  _dofShader(DoFShader),
																	      _outputFBO(result)
{
	F32 width = _resolution.width;
	F32 height = _resolution.height;
	_tempDepthOfFieldFBO = GFX_DEVICE.newFBO(FBO_2D_COLOR);
	_tempDepthOfFieldFBO->Create(width,height);
	_outputFBO->Create(width, height);
}

DoFPreRenderOperator::~DoFPreRenderOperator() {
	SAFE_DELETE(_tempDepthOfFieldFBO);
}

void DoFPreRenderOperator::reshape(I32 width, I32 height){
	if(_tempDepthOfFieldFBO){
		_tempDepthOfFieldFBO->Create(width,height);
	}
	_outputFBO->Create(width, height);
}


void DoFPreRenderOperator::operation(){
	if(!_enabled) return;
	if(!_renderQuad) return;
	if(_inputFBO.empty() || _inputFBO.size() != 2){
		ERROR_FN(Locale::get("ERROR_DOF_INPUT_FBO"));
	}
	GFXDevice& gfx = GFX_DEVICE;
	ParamHandler& par = ParamHandler::getInstance();

	gfx.toggle2D(true);
	_tempDepthOfFieldFBO->Begin();
	
		_dofShader->bind();

			_inputFBO[0]->Bind(0); ///screenFBO
			_inputFBO[1]->Bind(1); ///depthFBO
			_dofShader->Uniform("bHorizontal", false);
			_dofShader->UniformTexture("texScreen", 0);
			_dofShader->UniformTexture("texDepth",1);
		
			gfx.renderModel(_renderQuad);
				
			_inputFBO[1]->Unbind(1);
			_inputFBO[0]->Unbind(0);
		
	_tempDepthOfFieldFBO->End();

	
	_outputFBO->Begin();

		_dofShader->bind();
				
			_tempDepthOfFieldFBO->Bind(0);
			_inputFBO[1]->Bind(1);

			_dofShader->Uniform("bHorizontal", true);
			_dofShader->UniformTexture("texScreen", 0);
			_dofShader->UniformTexture("texDepth",1);
					
			gfx.renderModel(_renderQuad);
				
			_inputFBO[1]->Unbind(1);
			_tempDepthOfFieldFBO->Unbind(0);
				
	_outputFBO->End();
	gfx.toggle2D(false);
}