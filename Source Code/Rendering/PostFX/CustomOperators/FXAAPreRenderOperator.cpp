#include "Headers/FXAAPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"

FXAAPreRenderOperator::FXAAPreRenderOperator(FrameBuffer* result,
                                             const vec2<U16>& resolution,
                                             SamplerDescriptor* const sampler) : PreRenderOperator(FXAA_STAGE,resolution,sampler),
                                                                                 _outputFB(result),
                                                                                 _ready(false)
{
    _samplerCopy = GFX_DEVICE.newFB();
    TextureDescriptor fxaaDescriptor(TEXTURE_2D, RGBA8, UNSIGNED_BYTE);
    fxaaDescriptor.setSampler(*_internalSampler);

    _samplerCopy->AddAttachment(fxaaDescriptor,TextureDescriptor::Color0);
    _samplerCopy->toggleDepthBuffer(false);
    ResourceDescriptor fxaa("FXAA");
    fxaa.setThreadedLoading(false);
    _fxaa = CreateResource<ShaderProgram>(fxaa);
    _fxaa->UniformTexture("texScreen", 0);
    reshape(resolution.width, resolution.height);
}

FXAAPreRenderOperator::~FXAAPreRenderOperator(){
    RemoveResource(_fxaa);
    SAFE_DELETE(_samplerCopy);
}

void FXAAPreRenderOperator::reshape(I32 width, I32 height){
    _samplerCopy->Create(width,height);
    _fxaa->Uniform("size", vec2<F32>(_outputFB->getWidth(), _outputFB->getHeight()));
}

///This is tricky as we use our screen as both input and output
void FXAAPreRenderOperator::operation(){
    if (!_ready){
        if (!_enabled) return;
        _ready = true;
        return;
    }
    
    //Copy current screen
    _samplerCopy->BlitFrom(_inputFB[0]);
    ///Apply FXAA to the output screen using the sampler copy as the texture input
    _outputFB->Begin(FrameBuffer::defaultPolicy());
    _samplerCopy->Bind(0);
    _fxaa->bind();
    GFX_DEVICE.drawPoints(1);
    _outputFB->End();
}