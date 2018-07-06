#include "Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

SSAOPreRenderOperator::SSAOPreRenderOperator(Quad3D* target,
                                             FrameBuffer* result,
                                             const vec2<U16>& resolution,
                                             SamplerDescriptor* const sampler) : PreRenderOperator(SSAO_STAGE,target,resolution,sampler),
                                                                                 _outputFB(result)
{
    TextureDescriptor outputDescriptor(TEXTURE_2D,
                                       RGB,
                                       RGB8,
                                       UNSIGNED_BYTE);
    outputDescriptor.setSampler(*_internalSampler);
    _outputFB->AddAttachment(outputDescriptor,TextureDescriptor::Color0);
    _outputFB->Create(_resolution.width, _resolution.height);
    ResourceDescriptor ssao("SSAOPass");
    ssao.setThreadedLoading(false);
    _ssaoShader = CreateResource<ShaderProgram>(ssao);
    _ssaoShader->UniformTexture("texScreen", 0);
    _ssaoShader->UniformTexture("texDepth", 1);
}

SSAOPreRenderOperator::~SSAOPreRenderOperator(){
    RemoveResource(_ssaoShader);
}

void SSAOPreRenderOperator::reshape(I32 width, I32 height){
    _outputFB->Create(width, height);
}

void SSAOPreRenderOperator::operation(){
    if(!_enabled || !_renderQuad) return;

    GFXDevice& gfx = GFX_DEVICE;

    gfx.toggle2D(true);

    _ssaoShader->bind();
    _renderQuad->setCustomShader(_ssaoShader);

    _outputFB->Begin(FrameBuffer::defaultPolicy());
        _inputFB[0]->Bind(0); // screen
        _inputFB[1]->Bind(1, TextureDescriptor::Depth); // depth

            gfx.renderInstance(_renderQuad->renderInstance());

        _inputFB[1]->Unbind(1);
        _inputFB[0]->Unbind(0);

    _outputFB->End();

    gfx.toggle2D(false);
}