#include "Headers/FXAAPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"

namespace Divide {

FXAAPreRenderOperator::FXAAPreRenderOperator(Framebuffer* result,
                                             const vec2<U16>& resolution,
                                             SamplerDescriptor* const sampler)
    : PreRenderOperator(PostFXRenderStage::FXAA, resolution, sampler),
      _outputFB(result),
      _ready(false) {
    _samplerCopy = GFX_DEVICE.newFB();
    TextureDescriptor fxaaDescriptor(TextureType::TEXTURE_2D,
                                     GFXImageFormat::RGBA8,
                                     GFXDataFormat::UNSIGNED_BYTE);
    fxaaDescriptor.setSampler(*_internalSampler);

    _samplerCopy->addAttachment(fxaaDescriptor, TextureDescriptor::AttachmentType::Color0);
    _samplerCopy->toggleDepthBuffer(false);
    ResourceDescriptor fxaa("FXAA");
    fxaa.setThreadedLoading(false);
    _fxaa = CreateResource<ShaderProgram>(fxaa);
    reshape(resolution.width, resolution.height);
}

FXAAPreRenderOperator::~FXAAPreRenderOperator() {
    RemoveResource(_fxaa);
    MemoryManager::DELETE(_samplerCopy);
}

void FXAAPreRenderOperator::reshape(U16 width, U16 height) {
    _samplerCopy->create(width, height);
    _fxaa->Uniform("dvd_fxaaSpanMax", 8.0f);
    _fxaa->Uniform("dvd_fxaaReduceMul", 1.0f / 8.0f);
    _fxaa->Uniform("dvd_fxaaReduceMin", 1.0f / 128.0f);
    _fxaa->Uniform("dvd_fxaaSubpixShift", 1.0f / 4.0f);
}

/// This is tricky as we use our screen as both input and output
void FXAAPreRenderOperator::operation() {
    if (!_ready) {
        if (!_enabled) return;
        _ready = true;
        return;
    }

    // Copy current screen
    _samplerCopy->blitFrom(_inputFB[0]);
    // Apply FXAA to the output screen using the sampler copy as the texture
    // input
    _outputFB->begin(Framebuffer::defaultPolicy());
    _samplerCopy->bind(0);
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _fxaa);
    _outputFB->end();
}
};