#include "Headers/BloomPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h" 
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"

BloomPreRenderOperator::BloomPreRenderOperator(ShaderProgram* const bloomShader, 
											   Quad3D* target, 
											   FrameBufferObject* result, 
											   const vec2<U16>& resolution) : PreRenderOperator(BLOOM_STAGE,target,resolution),
																			 _blur(bloomShader),
																	         _outputFBO(result)
{
	U16 width = _resolution.width;
	U16 height = _resolution.height;
	_tempBloomFBO = GFX_DEVICE.newFBO(FBO_2D_COLOR);
	_tempBloomFBO->Create(width/4,height/4);
	_outputFBO->Create(width, height);
	_bright = CreateResource<ShaderProgram>(ResourceDescriptor("bright"));

}

BloomPreRenderOperator::~BloomPreRenderOperator(){
	RemoveResource(_bright);
	SAFE_DELETE(_tempBloomFBO);
}

void BloomPreRenderOperator::reshape(I32 width, I32 height){
	I32 w = width/4;
	I32 h = height/4;
	if(_tempBloomFBO){
		_tempBloomFBO->Create(w,h);
	}
	_outputFBO->Create(w, h);
}

void BloomPreRenderOperator::operation(){
	if(!_enabled) return;
	if(!_renderQuad) return;
	if(_inputFBO.empty()){
		ERROR_FN(Locale::get("ERROR_BLOOM_INPUT_FBO"));
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

			_outputFBO->Unbind(0);
		
		
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
		
		
	_outputFBO->End();

	gfx.toggle2D(false);
}