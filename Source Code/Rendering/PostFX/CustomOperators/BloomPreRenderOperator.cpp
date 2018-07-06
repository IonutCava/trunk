#include "Headers/BloomPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

BloomPreRenderOperator::BloomPreRenderOperator(RenderTarget* hdrTarget, RenderTarget* ldrTarget)
    : PreRenderOperator(FilterType::FILTER_BLOOM, hdrTarget, ldrTarget)
{
    for (U8 i = 0; i < 2; ++i) {
        _bloomBlurBuffer[i] = GFX_DEVICE.newRT();
        _bloomBlurBuffer[i]->addAttachment(_hdrTarget->getDescriptor(), TextureDescriptor::AttachmentType::Colour0);
        _bloomBlurBuffer[i]->useAutoDepthBuffer(false);
        _bloomBlurBuffer[i]->setClearColour(DefaultColours::BLACK());
    }

    _bloomOutput = GFX_DEVICE.newRT();
    _bloomOutput->addAttachment(_hdrTarget->getDescriptor(), TextureDescriptor::AttachmentType::Colour0);
    _bloomOutput->useAutoDepthBuffer(false);
    _bloomOutput->setClearColour(DefaultColours::BLACK());

    ResourceDescriptor bloomCalc("bloom.BloomCalc");
    bloomCalc.setThreadedLoading(false);
    _bloomCalc = CreateResource<ShaderProgram>(bloomCalc);

    ResourceDescriptor bloomApply("bloom.BloomApply");
    bloomApply.setThreadedLoading(false);
    _bloomApply = CreateResource<ShaderProgram>(bloomApply);

    ResourceDescriptor blur("blur");
    blur.setThreadedLoading(false);
    _blur = CreateResource<ShaderProgram>(blur);

    _blur->Uniform("kernelSize", 10);
    _horizBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurHorizontal");
    _vertBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurVertical");
}

BloomPreRenderOperator::~BloomPreRenderOperator() {
    MemoryManager::DELETE(_bloomOutput);
    MemoryManager::DELETE(_bloomBlurBuffer[0]);
    MemoryManager::DELETE(_bloomBlurBuffer[1]);
}

void BloomPreRenderOperator::idle() {
    _bloomApply->Uniform("bloomFactor",
                         ParamHandler::instance().getParam<F32>(_ID("postProcessing.bloomFactor"), 0.8f));
}

void BloomPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    U16 w = to_ushort(width / 4.0f);
    U16 h = to_ushort(height / 4.0f);
    _bloomOutput->create(w, h);
    _bloomBlurBuffer[0]->create(width, height);
    _bloomBlurBuffer[1]->create(width, height);

    _blur->Uniform("size", vec2<F32>(width, height));
}

// Order: luminance calc -> bloom -> tonemap
void BloomPreRenderOperator::execute() {
    size_t defaultStateHash = GFX_DEVICE.getDefaultStateBlock(true);

     // Step 1: generate bloom
    _hdrTarget->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0)); //screen
    // render all of the "bright spots"
    _bloomOutput->begin(RenderTarget::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _bloomCalc);
    _bloomOutput->end();

    // Step 2: blur bloom
    _blur->bind();
    // Blur horizontally
    _blur->SetSubroutine(ShaderType::FRAGMENT, _horizBlur);
    _bloomOutput->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0));
    _bloomBlurBuffer[0]->begin(RenderTarget::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _blur);
    _bloomBlurBuffer[0]->end();

    // Blur vertically (recycle the render target. We have a copy)
    _blur->SetSubroutine(ShaderType::FRAGMENT, _vertBlur);
    _bloomBlurBuffer[0]->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0));
    _bloomBlurBuffer[1]->begin(RenderTarget::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _blur);
    _bloomBlurBuffer[1]->end();
        
    // Step 3: apply bloom
    _bloomBlurBuffer[0]->blitFrom(_hdrTarget);
    _bloomBlurBuffer[0]->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0)); //Screen
    _bloomBlurBuffer[1]->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1),
                              TextureDescriptor::AttachmentType::Colour0); //Bloom
    _hdrTarget->begin(_screenOnlyDraw);
        GFX_DEVICE.drawTriangle(defaultStateHash, _bloomApply);
    _hdrTarget->end();
}

void BloomPreRenderOperator::debugPreview(U8 slot) const {
}

};