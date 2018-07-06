#include "Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

SSAOPreRenderOperator::SSAOPreRenderOperator(Quad3D* target,
											 FrameBufferObject* result,
											 const vec2<U16>& resolution,
											 SamplerDescriptor* const sampler) : PreRenderOperator(SSAO_STAGE,target,resolution,sampler),
																		         _outputFBO(result)
{

	TextureDescriptor outputDescriptor(TEXTURE_2D, 
		                               RGB,
									   RGB8,
									   UNSIGNED_BYTE);
	outputDescriptor.setSampler(*_internalSampler);
	_outputFBO->AddAttachment(outputDescriptor,TextureDescriptor::Color0);
	_outputFBO->Create(_resolution.width, _resolution.height);
	_ssaoShader = CreateResource<ShaderProgram>(ResourceDescriptor("SSAOPass"));
	_ssaoShader->UniformTexture("texScreen", 0);
	_ssaoShader->UniformTexture("texDepth", 1);
}

SSAOPreRenderOperator::~SSAOPreRenderOperator(){
	RemoveResource(_ssaoShader);
}

void SSAOPreRenderOperator::reshape(I32 width, I32 height){
	_outputFBO->Create(width, height);
}

void SSAOPreRenderOperator::operation(){
	if(!_enabled || !_renderQuad) return;

	GFXDevice& gfx = GFX_DEVICE;

	gfx.toggle2D(true);

	_ssaoShader->bind();
	_renderQuad->setCustomShader(_ssaoShader);

	_outputFBO->Begin();
		_inputFBO[0]->Bind(0); // screen
		_inputFBO[1]->Bind(1); // depth

			gfx.renderInstance(_renderQuad->renderInstance());

		_inputFBO[1]->Unbind(1);
		_inputFBO[0]->Unbind(0);

	_outputFBO->End();

	gfx.toggle2D(false);
}