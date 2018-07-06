#include "Headers/DoFPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

DoFPreRenderOperator::DoFPreRenderOperator(Quad3D* target,
                                           FrameBuffer* result,
                                           const vec2<U16>& resolution,
                                           SamplerDescriptor* const sampler) : PreRenderOperator(DOF_STAGE,target,resolution,sampler),
                                                                               _outputFB(result)
{
    TextureDescriptor dofDescriptor(TEXTURE_2D,
                                    RGBA,
                                    RGBA8,
                                    UNSIGNED_BYTE);
    dofDescriptor.setSampler(*_internalSampler);

    _samplerCopy->AddAttachment(dofDescriptor,TextureDescriptor::Color0);
    _samplerCopy->toggleDepthBuffer(false);
    _samplerCopy->Create(resolution.width,resolution.height);
    ResourceDescriptor dof("DepthOfField");
    dof.setThreadedLoading(false);
    _dofShader = CreateResource<ShaderProgram>(dof);
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

    if(_inputFB.empty()){
        ERROR_FN(Locale::get("ERROR_DOF_INPUT_FB"));
    }

    //Copy current screen
    _samplerCopy->BlitFrom(_inputFB[0]);

    GFXDevice& gfx = GFX_DEVICE;

    gfx.toggle2D(true);
    _outputFB->Begin(FrameBuffer::defaultPolicy());
        _dofShader->bind();
            _samplerCopy->Bind(0); //screenFB
            _inputFB[1]->Bind(1, TextureDescriptor::Depth); //depthFB
            _dofShader->UniformTexture("texScreen", 0);
            _dofShader->UniformTexture("texDepth",1);
            _renderQuad->setCustomShader(_dofShader);
            gfx.renderInstance(_renderQuad->renderInstance());

            _inputFB[1]->Unbind(1);
            _samplerCopy->Unbind(0);

    _outputFB->End();

    gfx.toggle2D(false);
}