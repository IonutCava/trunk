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
																			       _outputFBO(result),
																				   _tempHDRFBO(NULL),
																				   _luminaMipLevel(0)
{
	_luminaFBO[0] = NULL;
	_luminaFBO[1] = NULL;
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
	_bright->UniformTexture("texExposure", 1);
	_bright->UniformTexture("texPrevExposure", 2);
	_blur->UniformTexture("texScreen", 0);
	_blur->Uniform("kernelSize", 10);
}

BloomPreRenderOperator::~BloomPreRenderOperator(){
	RemoveResource(_bright);
	RemoveResource(_blur);
	SAFE_DELETE(_tempBloomFBO);
	SAFE_DELETE(_luminaFBO[0]);
	SAFE_DELETE(_luminaFBO[1]);
	SAFE_DELETE(_tempHDRFBO);
}
 
U32 nextPOW2(U32 n) {
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;  
	return n;
}

void BloomPreRenderOperator::reshape(I32 width, I32 height){
	assert(_tempBloomFBO);
	I32 w = width / 4;
	I32 h = height / 4;
	_tempBloomFBO->Create(w,h);
	_outputFBO->Create(w, h);
	if(_genericFlag && _tempHDRFBO){
		_tempHDRFBO->Create(width, height);
		U32 lumaRez = nextPOW2(width / 3);
		// make the texture square sized and power of two
		_luminaFBO[0]->Create(lumaRez , lumaRez);
		_luminaFBO[1]->Create(lumaRez , lumaRez);
		_luminaMipLevel = 0;
		while(lumaRez>>=1) _luminaMipLevel++;
	}
}

void BloomPreRenderOperator::operation(){
	if(!_enabled || !_renderQuad) 
		return;
	
	if(_inputFBO.empty()){
		ERROR_FN(Locale::get("ERROR_BLOOM_INPUT_FBO"));
		assert(!_inputFBO.empty());
	}

	GFXDevice& gfx = GFX_DEVICE;
	RenderInstance* quadRI = _renderQuad->renderInstance();

	gfx.toggle2D(true);

	_bright->bind();
	_renderQuad->setCustomShader(_bright);

	
	toneMapScreen();
	
	_bright->Uniform("toneMap", false);
	
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

void BloomPreRenderOperator::toneMapScreen()
{
	if(!_genericFlag)
		return;

	if(!_tempHDRFBO){
		_tempHDRFBO = GFX_DEVICE.newFBO(FBO_2D_COLOR_MS);
		TextureDescriptor hdrDescriptor(TEXTURE_2D,
										RGBA,
				                        RGBA16F,
										FLOAT_16);
		hdrDescriptor.setSampler(*_internalSampler);
		_tempHDRFBO->AddAttachment(hdrDescriptor, TextureDescriptor::Color0);
		_tempHDRFBO->Create(_inputFBO[0]->getWidth(), _inputFBO[0]->getHeight());

		_luminaFBO[0] = GFX_DEVICE.newFBO(FBO_2D_COLOR);
		_luminaFBO[1] = GFX_DEVICE.newFBO(FBO_2D_COLOR);

		SamplerDescriptor lumaSampler;
		lumaSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
		lumaSampler.setMinFilter(TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR);
		lumaSampler.toggleMipMaps(true);

		TextureDescriptor lumaDescriptor(TEXTURE_2D,
										 RED,
										 RED16F,
										 FLOAT_16);
		lumaDescriptor.setSampler(lumaSampler);
		_luminaFBO[0]->AddAttachment(lumaDescriptor, TextureDescriptor::Color0);
		U32 lumaRez = nextPOW2(_inputFBO[0]->getWidth() / 3);
		// make the texture square sized and power of two
		_luminaFBO[0]->Create(lumaRez , lumaRez);

		lumaSampler.setFilters(TEXTURE_FILTER_LINEAR);
		lumaSampler.toggleMipMaps(false);
		lumaDescriptor.setSampler(lumaSampler);
		_luminaFBO[1]->AddAttachment(lumaDescriptor, TextureDescriptor::Color0);

		_luminaFBO[1]->Create(1, 1);
		_luminaMipLevel = 0;
		while(lumaRez>>=1) _luminaMipLevel++;
	}

	_bright->Uniform("luminancePass", true);
	
	_luminaFBO[1]->BlitFrom(_luminaFBO[0]);

	_luminaFBO[0]->Begin();
		_inputFBO[0]->Bind(0);
		_luminaFBO[1]->Bind(2);
			GFX_DEVICE.renderInstance(_renderQuad->renderInstance());
		_luminaFBO[1]->Unbind(2);
		_inputFBO[0]->Unbind(0);
	_luminaFBO[0]->End();

	_bright->Uniform("toneMap", true);
	_bright->Uniform("luminancePass", false);

	_tempHDRFBO->BlitFrom(_inputFBO[0]);

	_inputFBO[0]->Begin();
	   //screen FBO
		_tempHDRFBO->Bind(0);
		// luminance FBO
		_luminaFBO[0]->Bind(1);
		_luminaFBO[0]->UpdateMipMaps(TextureDescriptor::Color0);
			GFX_DEVICE.renderInstance(_renderQuad->renderInstance());
		_luminaFBO[0]->Unbind(1);
		_tempHDRFBO->Unbind(0);
	_inputFBO[0]->End();
}