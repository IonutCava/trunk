#include "Headers/BloomPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"

BloomPreRenderOperator::BloomPreRenderOperator(Quad3D* target,
											   FrameBufferObject* result,
											   const vec2<U16>& resolution) : PreRenderOperator(BLOOM_STAGE,target,resolution),
																			  _outputFBO(result)
{
	F32 width = _resolution.width;
	F32 height = _resolution.height;
	_tempBloomFBO = GFX_DEVICE.newFBO(FBO_2D_COLOR);

	TextureDescriptor tempBloomDescriptor(TEXTURE_2D, RGBA,RGBA8,FLOAT_32);
	tempBloomDescriptor.setWrapMode(TEXTURE_CLAMP_TO_EDGE,TEXTURE_CLAMP_TO_EDGE);
	tempBloomDescriptor._generateMipMaps = false; //it's a flat texture on a full screen quad. really?
	_tempBloomFBO->AddAttachment(tempBloomDescriptor,TextureDescriptor::Color0);
	_tempBloomFBO->Create(width/4,height/4);

	TextureDescriptor outputBloomDescriptor(TEXTURE_2D, RGBA,RGBA8,FLOAT_32);
	outputBloomDescriptor.setWrapMode(TEXTURE_CLAMP_TO_EDGE,TEXTURE_CLAMP_TO_EDGE);
	outputBloomDescriptor._generateMipMaps = false; //it's a flat texture on a full screen quad. really?
	_outputFBO->AddAttachment(tempBloomDescriptor,TextureDescriptor::Color0);
	_outputFBO->Create(width, height);
	_bright = CreateResource<ShaderProgram>(ResourceDescriptor("bright"));
	_blur = CreateResource<ShaderProgram>(ResourceDescriptor("blur"));
}

BloomPreRenderOperator::~BloomPreRenderOperator(){
	RemoveResource(_bright);
	RemoveResource(_blur);
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

	gfx.toggle2D(true);
	_outputFBO->Begin();

		_bright->bind();
        _renderQuad->setCustomShader(_bright);
			//screen FBO
			_inputFBO[0]->Bind(0);

				_bright->UniformTexture("texScreen", 0);

				gfx.renderInstance(_renderQuad->renderInstance());

			_inputFBO[0]->Unbind(0);

	_outputFBO->End();

	//Blur horizontally
	_tempBloomFBO->Begin();

		_blur->bind();
		_renderQuad->setCustomShader(_blur);
			_outputFBO->Bind(0);
				_blur->UniformTexture("texScreen", 0);
				_blur->Uniform("size", vec2<F32>((F32)_outputFBO->getWidth(), (F32)_outputFBO->getHeight()));
				_blur->Uniform("horizontal", true);
				_blur->Uniform("kernel_size", 10);

				gfx.renderInstance(_renderQuad->renderInstance());

			_outputFBO->Unbind(0);

	_tempBloomFBO->End();

	//Blur vertically
	_outputFBO->Begin();
		_blur->bind();

			_tempBloomFBO->Bind(0);

				_blur->UniformTexture("texScreen", 0);
				_blur->Uniform("size", vec2<F32>((F32)_tempBloomFBO->getWidth(), (F32)_tempBloomFBO->getHeight()));
				_blur->Uniform("horizontal", false);

				gfx.renderInstance(_renderQuad->renderInstance());

			_tempBloomFBO->Unbind(0);

	_outputFBO->End();

	gfx.toggle2D(false);
}