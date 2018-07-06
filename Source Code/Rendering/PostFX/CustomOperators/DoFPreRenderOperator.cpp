#include "Headers/DoFPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

DoFPreRenderOperator::DoFPreRenderOperator(Framebuffer* result,
                                           const vec2<U16>& resolution,
                                           SamplerDescriptor* const sampler) : PreRenderOperator(DOF_STAGE,resolution,sampler),
                                                                               _outputFB(result)
{
    TextureDescriptor dofDescriptor(TEXTURE_2D, RGBA8, UNSIGNED_BYTE);
    dofDescriptor.setSampler(*_internalSampler);

    _samplerCopy->AddAttachment(dofDescriptor,TextureDescriptor::Color0);
    _samplerCopy->toggleDepthBuffer(false);
    _samplerCopy->Create(resolution.width,resolution.height);
    ResourceDescriptor dof("DepthOfField");
    dof.setThreadedLoading(false);
    _dofShader = CreateResource<ShaderProgram>(dof);
    _dofShader->UniformTexture("texScreen", 0);
    _dofShader->UniformTexture("texDepth", 1);
}

DoFPreRenderOperator::~DoFPreRenderOperator()
{
    RemoveResource(_dofShader);
    MemoryManager::DELETE( _samplerCopy );
}

void DoFPreRenderOperator::reshape(I32 width, I32 height){
    _samplerCopy->Create(width,height);
}

void DoFPreRenderOperator::operation(){
    if(!_enabled) return;

    if(_inputFB.empty()){
        Console::errorfn(Locale::get("ERROR_DOF_INPUT_FB"));
        return;
    }

    //Copy current screen
    _samplerCopy->BlitFrom(_inputFB[0]);

    _outputFB->Begin(Framebuffer::defaultPolicy());
    _samplerCopy->Bind(0); //screenFB
    _inputFB[1]->Bind(1, TextureDescriptor::Depth); //depthFB
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _dofShader);
    _outputFB->End();
}

};