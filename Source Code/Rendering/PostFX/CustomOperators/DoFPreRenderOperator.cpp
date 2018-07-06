#include "Headers/DoFPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

DoFPreRenderOperator::DoFPreRenderOperator(Quad3D* target,
										   FrameBufferObject* result,
										   const vec2<U16>& resolution,
										   SamplerDescriptor* const sampler) : PreRenderOperator(DOF_STAGE,target,resolution,sampler),
																	           _outputFBO(result)
{
    TextureDescriptor dofDescriptor(TEXTURE_2D,
		                            RGBA,
									RGBA8,
									UNSIGNED_BYTE);
	dofDescriptor.setSampler(*_internalSampler);

	_samplerCopy->AddAttachment(dofDescriptor,TextureDescriptor::Color0);
	_samplerCopy->toggleDepthBuffer(false);
	_samplerCopy->Create(resolution.width,resolution.height);

	_dofShader = CreateResource<ShaderProgram>(ResourceDescriptor("DepthOfField"));
}

DoFPreRenderOperator::~DoFPreRenderOperator() {
	RemoveResource(_dofShader);
	SAFE_DELETE(_samplerCopy);
}

void DoFPreRenderOperator::reshape(I32 width, I32 height){
	_samplerCopy->Create(width,height);
}

void DoFPreRenderOperator::operation(){
	if(!_enabled || !_renderQuad) return;

	if(_inputFBO.empty()){
		ERROR_FN(Locale::get("ERROR_DOF_INPUT_FBO"));
	}

	//Copy current screen
	_samplerCopy->BlitFrom(_inputFBO[0]);

	GFXDevice& gfx = GFX_DEVICE;

	gfx.toggle2D(true);
	_outputFBO->Begin();
		_dofShader->bind();
			_samplerCopy->Bind(0); //screenFBO
			_inputFBO[1]->Bind(1); //depthFBO
			_dofShader->UniformTexture("texScreen", 0);
			_dofShader->UniformTexture("texDepth",1);
		    _renderQuad->setCustomShader(_dofShader);
			gfx.renderInstance(_renderQuad->renderInstance());

			_inputFBO[1]->Unbind(1);
			_samplerCopy->Unbind(0);

	_outputFBO->End();

	gfx.toggle2D(false);
}