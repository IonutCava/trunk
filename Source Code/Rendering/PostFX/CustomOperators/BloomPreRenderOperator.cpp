#include "Headers/BloomPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

namespace {
    inline U32 nextPOW2(U32 n) {
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n++;
        return n;
    }
};

BloomPreRenderOperator::BloomPreRenderOperator(Framebuffer* hdrTarget, Framebuffer* ldrTarget)
    : PreRenderOperator(FilterType::FILTER_BLOOM_TONEMAP, hdrTarget, ldrTarget)
{
    for (U8 i = 0; i < 2; ++i) {
        _bloomBlurBuffer[i] = GFX_DEVICE.newFB();
        _bloomBlurBuffer[i]->addAttachment(_hdrTarget->getDescriptor(), TextureDescriptor::AttachmentType::Color0);
        _bloomBlurBuffer[i]->toggleDepthBuffer(false);
        _bloomBlurBuffer[i]->setClearColor(DefaultColors::BLACK());
    }

    _bloomOutput = GFX_DEVICE.newFB();
    _bloomOutput->addAttachment(_hdrTarget->getDescriptor(), TextureDescriptor::AttachmentType::Color0);
    _bloomOutput->toggleDepthBuffer(false);
    _bloomOutput->setClearColor(DefaultColors::BLACK());

    _previousExposure = GFX_DEVICE.newFB();
    _currentExposure = GFX_DEVICE.newFB();

    SamplerDescriptor lumaSampler;
    lumaSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    lumaSampler.setMinFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    lumaSampler.toggleMipMaps(true);

    TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D,
                                    GFXImageFormat::RED16F,
                                    GFXDataFormat::FLOAT_16);
    lumaDescriptor.setSampler(lumaSampler);
    _currentExposure->addAttachment(lumaDescriptor, TextureDescriptor::AttachmentType::Color0);
    lumaSampler.setFilters(TextureFilter::LINEAR);
    lumaDescriptor.setSampler(lumaSampler);
    _previousExposure->addAttachment(lumaDescriptor, TextureDescriptor::AttachmentType::Color0);
    _previousExposure->setClearColor(DefaultColors::BLACK());

    ResourceDescriptor bloom("bloom");
    bloom.setThreadedLoading(false);
    _bloom = CreateResource<ShaderProgram>(bloom);

    ResourceDescriptor bloomApply("bloomApply");
    bloomApply.setThreadedLoading(false);
    _bloomApply = CreateResource<ShaderProgram>(bloomApply);

    ResourceDescriptor blur("blur");
    blur.setThreadedLoading(false);
    _blur = CreateResource<ShaderProgram>(blur);

    ResourceDescriptor luminanceCalc("luminanceCalc");
    luminanceCalc.setThreadedLoading(false);
    _luminanceCalc = CreateResource<ShaderProgram>(luminanceCalc);

    _blur->Uniform("kernelSize", 10);
    _horizBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurHorizontal");
    _vertBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurVertical");
}

BloomPreRenderOperator::~BloomPreRenderOperator() {
    RemoveResource(_bloom);
    RemoveResource(_bloomApply);
    RemoveResource(_blur);

    MemoryManager::DELETE(_bloomOutput);
    MemoryManager::DELETE(_bloomBlurBuffer[0]);
    MemoryManager::DELETE(_bloomBlurBuffer[1]);
    MemoryManager::DELETE(_previousExposure);
    MemoryManager::DELETE(_currentExposure);
}

void BloomPreRenderOperator::idle() {
    _bloomApply->Uniform("bloomFactor",
                         ParamHandler::getInstance().getParam<F32>(_ID("postProcessing.bloomFactor"), 0.8));
}

void BloomPreRenderOperator::reshape(U16 width, U16 height) {
    U16 w = to_ushort(width / 4.0f);
    U16 h = to_ushort(height / 4.0f);
    _bloomOutput->create(w, h);
    _bloomBlurBuffer[0]->create(width, height);
    _bloomBlurBuffer[1]->create(width, height);

    U16 lumaRez = to_ushort(nextPOW2(to_uint(width / 3.0f)));
    U16 lumaRezCopy = lumaRez;
    I32 luminaMipLevel = 0;
    while (lumaRez >>= 1) {
        luminaMipLevel++;
    }

    // make the texture square sized and power of two
    _currentExposure->create(lumaRezCopy, lumaRezCopy);
    _previousExposure->create(1, 1);

    _blur->Uniform("size", vec2<F32>(width, height));
    _bloom->Uniform("exposureMipLevel", luminaMipLevel);

    _previousExposure->clear();
    _currentExposure->clear();
}

// Order: luminance calc -> bloom -> tonemap
void BloomPreRenderOperator::execute() {
    U32 defaultStateHash = GFX_DEVICE.getDefaultStateBlock(true);

    // Step 1: Luminance calc
    _hdrTarget->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
    _previousExposure->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT1));

    _currentExposure->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _luminanceCalc);
    _currentExposure->end();

    // Use previous exposure to control adaptive luminance
    _previousExposure->blitFrom(_currentExposure);

     // Step 2: generate bloom
    _bloom->Uniform("toneMap", false);
    _hdrTarget->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0)); //screen
    // render all of the "bright spots"
    _bloomOutput->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _bloom);
    _bloomOutput->end();

    _blur->bind();
    // Blur horizontally
    _blur->SetSubroutine(ShaderType::FRAGMENT, _horizBlur);
    _bloomOutput->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
    _bloomBlurBuffer[0]->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _blur);
    _bloomBlurBuffer[0]->end();

    // Blur vertically
    _blur->SetSubroutine(ShaderType::FRAGMENT, _vertBlur);
    _bloomBlurBuffer[0]->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
    _bloomBlurBuffer[1]->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _blur);
    _bloomBlurBuffer[1]->end();
        
    // Step 3: apply bloom
    _hdrTarget->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
    _bloomBlurBuffer[1]->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT1));
    _bloomBlurBuffer[0]->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _bloomApply);
    _bloomBlurBuffer[0]->end();

    // Step 4:  tone map
    _bloom->Uniform("toneMap", true);
    _bloomBlurBuffer[0]->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
    _currentExposure->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT1));
    _ldrTarget->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(defaultStateHash, _bloom);
    _ldrTarget->end();
    ldrTargetValid(true);
}

void BloomPreRenderOperator::debugPreview(U8 slot) const {
    _previousExposure->bind(slot);
}

};