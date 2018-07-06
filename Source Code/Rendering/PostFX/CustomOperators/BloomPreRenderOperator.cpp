#include "Headers/BloomPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"

BloomPreRenderOperator::BloomPreRenderOperator(Quad3D* target,
											   FrameBufferObject* result,
											   const vec2<U16>& resolution,
											   SamplerDescriptor* const sampler) : PreRenderOperator(BLOOM_STAGE,target,resolution,sampler),
																			       _outputFBO(result)
{
	F32 width = _resolution.width;
	F32 height = _resolution.height;
	_tempBloomFBO = GFX_DEVICE.newFBO(FBO_2D_COLOR);

	TextureDescriptor tempBloomDescriptor(TEXTURE_2D, 
		                                  RGB,
										  RGB8,
										  UNSIGNED_BYTE);
	tempBloomDescriptor.setSampler(*_internalSampler);

	_tempBloomFBO->AddAttachment(tempBloomDescriptor,TextureDescriptor::Color0);
	_tempBloomFBO->Create(width/4,height/4);
	_outputFBO->AddAttachment(tempBloomDescriptor, TextureDescriptor::Color0);
	_outputFBO->Create(width, height);
	_bright = CreateResource<ShaderProgram>(ResourceDescriptor("bright"));
	_blur = CreateResource<ShaderProgram>(ResourceDescriptor("blur"));

	_bright->UniformTexture("texScreen", 0);
	_blur->UniformTexture("texScreen", 0);
	_blur->Uniform("kernelSize", 10);
}

BloomPreRenderOperator::~BloomPreRenderOperator(){
	RemoveResource(_bright);
	RemoveResource(_blur);
	SAFE_DELETE(_tempBloomFBO);
}

void BloomPreRenderOperator::reshape(I32 width, I32 height){
	assert(_tempBloomFBO);
	I32 w = width / 4;
	I32 h = height / 4;

	_tempBloomFBO->Create(w,h);
	_outputFBO->Create(w, h);
}

void BloomPreRenderOperator::operation(){
	if(!_enabled || !_renderQuad) return;

	if(_inputFBO.empty()){
		ERROR_FN(Locale::get("ERROR_BLOOM_INPUT_FBO"));
		assert(!_inputFBO.empty());
	}
	GFXDevice& gfx = GFX_DEVICE;
	RenderInstance* quadRI = _renderQuad->renderInstance();

	gfx.toggle2D(true);
	
	_bright->bind();
	_renderQuad->setCustomShader(_bright);
	
	// render all of the "bright spots"
	_outputFBO->Begin();
        //screen FBO
		_inputFBO[0]->Bind(0);
			gfx.renderInstance(quadRI);
		_inputFBO[0]->Unbind(0);
	_outputFBO->End();

	_blur->bind();
	_blur->Uniform("size", vec2<F32>((F32)_outputFBO->getWidth(), (F32)_outputFBO->getHeight()));
	_blur->Uniform("horizontal", true);
	_renderQuad->setCustomShader(_blur);

	//Blur horizontally
	_tempBloomFBO->Begin();
	    //bright spots
		_outputFBO->Bind(0);
			gfx.renderInstance(quadRI);
		_outputFBO->Unbind(0);
	_tempBloomFBO->End();

	_blur->Uniform("size", vec2<F32>((F32)_tempBloomFBO->getWidth(), (F32)_tempBloomFBO->getHeight()));
	_blur->Uniform("horizontal", false);

	//Blur vertically
	_outputFBO->Begin();
		//horizontally blurred bright spots
		_tempBloomFBO->Bind(0);
			gfx.renderInstance(quadRI);
		_tempBloomFBO->Unbind(0);
	_outputFBO->End();

	gfx.toggle2D(false);
}