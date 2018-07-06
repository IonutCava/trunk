#include "Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"
#include "Managers/Headers/ResourceManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h" 
#include "Hardware/Video/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"


SSAOPreRenderOperator::SSAOPreRenderOperator(ShaderProgram* const SSAOShader, Quad3D* target, FrameBufferObject* result) : PreRenderOperator(SSAO_STAGE,target),
												_stage1Shader(SSAOShader),
												_outputFBO(result)
{
	F32 width = Application::getInstance().getWindowDimensions().width;
	F32 height = Application::getInstance().getWindowDimensions().height;
	ParamHandler& par = ParamHandler::getInstance();
	ResourceDescriptor colorNoiseTexture("noiseTexture");
	colorNoiseTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//noise.png");
	_colorNoise = ResourceManager::getInstance().loadResource<Texture>(colorNoiseTexture);
	_normalsFBO = GFXDevice::getInstance().newFBO();
	_normalsFBO->Create(FrameBufferObject::FBO_2D_COLOR, width,height);
	_stage2Shader = ResourceManager::getInstance().loadResource<ShaderProgram>(ResourceDescriptor("SSAOPass2"));

}

SSAOPreRenderOperator::~SSAOPreRenderOperator(){
	RemoveResource(_stage2Shader);
	RemoveResource(_colorNoise);
	if(_normalsFBO){
		delete _normalsFBO;
		_normalsFBO = NULL;
	}
}

void SSAOPreRenderOperator::reshape(I32 width, I32 height){
	if(_normalsFBO){
		_normalsFBO->Create(FrameBufferObject::FBO_2D_COLOR, width,height);
	}
}

void SSAOPreRenderOperator::operation(){
	if(!_enabled) return;
	if(!_renderQuad) return;
	GFXDevice& gfx = GFXDevice::getInstance();
	ParamHandler& par = ParamHandler::getInstance();

	_normalsFBO->Begin();
		_stage1Shader->bind();

				SceneManager::getInstance().render(SSAO_STAGE);

		//_stage1Shader->unbind();

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
		//_stage2Shader->unbind();
	_outputFBO->End();
}