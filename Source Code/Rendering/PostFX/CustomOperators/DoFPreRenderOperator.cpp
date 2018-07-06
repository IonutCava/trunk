#include "Headers/DoFPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

DoFPreRenderOperator::DoFPreRenderOperator(Framebuffer* result,
                                           const vec2<U16>& resolution,
                                           SamplerDescriptor* const sampler)
    : PreRenderOperator(PostFXRenderStage::DOF, resolution, sampler), _outputFB(result)
{
    TextureDescriptor dofDescriptor(TextureType::TEXTURE_2D,
                                    GFXImageFormat::RGBA8,
                                    GFXDataFormat::UNSIGNED_BYTE);
    dofDescriptor.setSampler(*_internalSampler);

    _samplerCopy = GFX_DEVICE.newFB();
    _samplerCopy->addAttachment(dofDescriptor, TextureDescriptor::AttachmentType::Color0);
    _samplerCopy->toggleDepthBuffer(false);
    _samplerCopy->create(resolution.width, resolution.height);
    ResourceDescriptor dof("DepthOfField");
    dof.setThreadedLoading(false);
    _dofShader = CreateResource<ShaderProgram>(dof);
    _dofShader->Uniform("texScreen", ShaderProgram::TextureUsage::UNIT0);
    _dofShader->Uniform("texDepth", ShaderProgram::TextureUsage::UNIT1);
}

DoFPreRenderOperator::~DoFPreRenderOperator()
{
    RemoveResource(_dofShader);
    MemoryManager::DELETE(_samplerCopy);
}

void DoFPreRenderOperator::reshape(U16 width, U16 height) {
    _samplerCopy->create(width, height);
}

void DoFPreRenderOperator::operation() {
    if (!_enabled) {
        return;
    }

    if (_inputFB.empty()) {
        Console::errorfn(Locale::get("ERROR_DOF_INPUT_FB"));
        return;
    }

    // Copy current screen
    _samplerCopy->blitFrom(_inputFB[0]);

    _outputFB->begin(Framebuffer::defaultPolicy());
    _samplerCopy->bind(0);  // screenFB
    _inputFB[1]->bind(1, TextureDescriptor::AttachmentType::Depth);  // depthFB
    GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _dofShader);
    _outputFB->end();
}
};