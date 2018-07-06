#include "Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

SSAOPreRenderOperator::SSAOPreRenderOperator(Framebuffer* result,
                                             const vec2<U16>& resolution,
                                             SamplerDescriptor* const sampler)
    : PreRenderOperator(PostFXRenderStage::SSAO_STAGE, resolution, sampler), _outputFB(result) {
    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RGB8,
                                       GFXDataFormat::UNSIGNED_BYTE);
    outputDescriptor.setSampler(*_internalSampler);
    _outputFB->AddAttachment(outputDescriptor, TextureDescriptor::AttachmentType::Color0);
    _outputFB->Create(_resolution.width, _resolution.height);
    ResourceDescriptor ssao("SSAOPass");
    ssao.setThreadedLoading(false);
    _ssaoShader = CreateResource<ShaderProgram>(ssao);
    _ssaoShader->UniformTexture("texScreen", 0);
    _ssaoShader->UniformTexture("texDepth", 1);
}

SSAOPreRenderOperator::~SSAOPreRenderOperator() { RemoveResource(_ssaoShader); }

void SSAOPreRenderOperator::reshape(I32 width, I32 height) {
    _outputFB->Create(width, height);
}

void SSAOPreRenderOperator::operation() {
    if (!_enabled) return;

    _outputFB->Begin(Framebuffer::defaultPolicy());
    _inputFB[0]->Bind(0);                            // screen
    _inputFB[1]->Bind(1, TextureDescriptor::AttachmentType::Depth);  // depth
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true),
                          _ssaoShader);
    _outputFB->End();
}
};