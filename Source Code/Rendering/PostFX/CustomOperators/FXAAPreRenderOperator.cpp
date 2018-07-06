#include "Headers/FXAAPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"

FXAAPreRenderOperator::FXAAPreRenderOperator(Quad3D* target, 
											 FrameBufferObject* result, 
											 const vec2<U16>& resolution) : PreRenderOperator(FXAA_STAGE,target,resolution),
																	         _outputFBO(result)
{
	_samplerCopy = GFX_DEVICE.newFBO(FBO_2D_COLOR);
	TextureDescriptor fxaaDescriptor(TEXTURE_2D, RGBA,RGBA8,FLOAT_32);
	fxaaDescriptor.setWrapMode(TEXTURE_CLAMP_TO_EDGE,TEXTURE_CLAMP_TO_EDGE);
	fxaaDescriptor._generateMipMaps = false; //it's a flat texture on a full screen quad. really?
	_samplerCopy->AddAttachment(fxaaDescriptor,TextureDescriptor::Color0);
	_samplerCopy->toggleDepthWrites(false);
	_samplerCopy->Create(resolution.width,resolution.height);	
	_fxaa = CreateResource<ShaderProgram>(ResourceDescriptor("FXAA"));
}

FXAAPreRenderOperator::~FXAAPreRenderOperator(){
	RemoveResource(_fxaa);
	SAFE_DELETE(_samplerCopy);
}

void FXAAPreRenderOperator::reshape(I32 width, I32 height){
	_samplerCopy->Create(width,height);
}
///This is tricky as we use our screen as both input and output
void FXAAPreRenderOperator::operation(){
	if(!_enabled) return;
	if(!_renderQuad) return;

	GFXDevice& gfx = GFX_DEVICE;
	
	//Copy current screen
	_samplerCopy->BlitFrom(_inputFBO[0]);
	///Apply FXAA to the output screen using the sampler copy as the texture input
	_outputFBO->Begin();
		_samplerCopy->Bind(0);
		_fxaa->bind();
		_fxaa->UniformTexture("texScreen", 0);
		_fxaa->Uniform("size", vec2<F32>((F32)_outputFBO->getWidth(), (F32)_outputFBO->getHeight()));
			gfx.toggle2D(true);
			gfx.renderModel(_renderQuad);
			gfx.toggle2D(false);
		_samplerCopy->Unbind(0);
	_outputFBO->End();
}