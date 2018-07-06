#include "Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"
#include "Managers/Headers/ResourceManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h" 
#include "Hardware/Video/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"


BloomPreRenderOperator::BloomPreRenderOperator(ShaderProgram* const bloomShader, Quad3D* target, FrameBufferObject* result) : PreRenderOperator(BLOOM_STAGE,target),
												_blur(bloomShader),
												_outputFBO(result)
{
	F32 width = Application::getInstance().getWindowDimensions().width;
	F32 height = Application::getInstance().getWindowDimensions().height;
	I32 w = width/4, h = height/4;
	_tempBloomFBO = GFX_DEVICE.newFBO();
	_tempBloomFBO->Create(FBO_2D_COLOR,w,h);
	_bright = CreateResource<ShaderProgram>(ResourceDescriptor("bright"));

}

BloomPreRenderOperator::~BloomPreRenderOperator(){
	RemoveResource(_bright);
	SAFE_DELETE(_tempBloomFBO);
}

void BloomPreRenderOperator::reshape(I32 width, I32 height){
	if(_tempBloomFBO){
		_tempBloomFBO->Create(FBO_2D_COLOR, width/4,height/4);
	}
}

void BloomPreRenderOperator::operation(){
	if(!_enabled) return;
	if(!_renderQuad) return;
	if(_inputFBO.empty()){
		ERROR_FN("Bloom Operator - no input FBO");
	}
	GFXDevice& gfx = GFX_DEVICE;
	ParamHandler& par = ParamHandler::getInstance();

	gfx.toggle2D(true);
	_outputFBO->Begin();

		_bright->bind();

			//screen FBO
			_inputFBO[0]->Bind(0);
				
				_bright->UniformTexture("texScreen", 0);
				_bright->Uniform("threshold", 0.95f);

				gfx.renderModel(_renderQuad);

			_inputFBO[0]->Unbind(0);

		//_bright->unbind();

	_outputFBO->End();


	//Blur horizontally
	_tempBloomFBO->Begin();

		_blur->bind();
		
			_outputFBO->Bind(0);
				_blur->UniformTexture("texScreen", 0);
				_blur->Uniform("size", vec2<F32>((F32)_outputFBO->getWidth(), (F32)_outputFBO->getHeight()));
				_blur->Uniform("horizontal", true);
				_blur->Uniform("kernel_size", 10);

				gfx.renderModel(_renderQuad);

			//_outputFBO->Unbind(0);
		
		_blur->unbind();
		
	_tempBloomFBO->End();


	//Blur vertically
	_outputFBO->Begin();
		_blur->bind();
		
			_tempBloomFBO->Bind(0);

				_blur->UniformTexture("texScreen", 0);
				_blur->Uniform("size", vec2<F32>((F32)_tempBloomFBO->getWidth(), (F32)_tempBloomFBO->getHeight()));
				_blur->Uniform("horizontal", false);

				gfx.renderModel(_renderQuad);

			_tempBloomFBO->Unbind(0);
		
		//_blur->unbind();
		
	_outputFBO->End();

	gfx.toggle2D(false);
}