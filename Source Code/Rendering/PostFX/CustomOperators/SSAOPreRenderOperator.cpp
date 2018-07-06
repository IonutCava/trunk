#include "Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h" 
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"


SSAOPreRenderOperator::SSAOPreRenderOperator(Quad3D* target, 
											 FrameBufferObject* result,
											 const vec2<U16>& resolution) : PreRenderOperator(SSAO_STAGE,target,resolution),
																		    _outputFBO(result)
{
	F32 width = _resolution.width;
	F32 height = _resolution.height;
	ParamHandler& par = ParamHandler::getInstance();
	ResourceDescriptor colorNoiseTexture("noiseTexture");
	colorNoiseTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//noise.png");
	_colorNoise = CreateResource<Texture>(colorNoiseTexture);

	TextureDescriptor normalsDescriptor(TEXTURE_2D, RGBA,RGBA8,FLOAT_32);
	normalsDescriptor.setWrapMode(TEXTURE_CLAMP_TO_EDGE,TEXTURE_CLAMP_TO_EDGE);
	normalsDescriptor._generateMipMaps = false; //it's a flat texture on a full screen quad. really?
	_normalsFBO->AddAttachment(normalsDescriptor,TextureDescriptor::Color0);
	_normalsFBO->toggleDepthWrites(false);

	_normalsFBO = GFX_DEVICE.newFBO(FBO_2D_COLOR);
	_normalsFBO->Create(width,height);

	TextureDescriptor outputDescriptor(TEXTURE_2D, RGBA,RGBA8,FLOAT_32);
	outputDescriptor.setWrapMode(TEXTURE_CLAMP_TO_EDGE,TEXTURE_CLAMP_TO_EDGE);
	outputDescriptor._generateMipMaps = false; //it's a flat texture on a full screen quad. really?
	_outputFBO->AddAttachment(outputDescriptor,TextureDescriptor::Color0);
	_outputFBO->Create(width, height);
	_stage1Shader = CreateResource<ShaderProgram>(ResourceDescriptor("SSAOPass1"));
	_stage2Shader = CreateResource<ShaderProgram>(ResourceDescriptor("SSAOPass2"));

}

SSAOPreRenderOperator::~SSAOPreRenderOperator(){
	RemoveResource(_stage1Shader);
	RemoveResource(_stage2Shader);
	RemoveResource(_colorNoise);
	SAFE_DELETE(_normalsFBO);
}

void SSAOPreRenderOperator::reshape(I32 width, I32 height){
	if(_normalsFBO){
		_normalsFBO->Create(width,height);
	}
	_outputFBO->Create(width, height);
}

void SSAOPreRenderOperator::operation(){
	if(!_enabled) return;
	if(!_renderQuad) return;
	GFXDevice& gfx = GFX_DEVICE;
	ParamHandler& par = ParamHandler::getInstance();

	_normalsFBO->Begin();
		_stage1Shader->bind();

				SceneManager::getInstance().render(SSAO_STAGE);

	_normalsFBO->End();

	_outputFBO->Begin();
		gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
		_stage2Shader->bind();
			_normalsFBO->Bind(0);
			_colorNoise->Bind(1);
			_stage2Shader->UniformTexture("normalMap",0);
			_stage2Shader->UniformTexture("rnm",1);
			_stage2Shader->Uniform("totStrength",  1.38f);
			_stage2Shader->Uniform("strength", 0.07f);
			_stage2Shader->Uniform("offset", 18.0f);
			_stage2Shader->Uniform("falloff", 0.000002f);
			_stage2Shader->Uniform("rad", 0.006f);

			gfx.toggle2D(true);
			gfx.renderModel(_renderQuad);
			gfx.toggle2D(false);

			_colorNoise->Unbind(1);
			_normalsFBO->Unbind(0);

	_outputFBO->End();
}