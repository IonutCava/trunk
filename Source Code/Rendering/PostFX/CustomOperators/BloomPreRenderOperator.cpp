#include "Headers/BloomPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"

namespace Divide {

BloomPreRenderOperator::BloomPreRenderOperator(Framebuffer* result,
                                               const vec2<U16>& resolution,
                                               SamplerDescriptor* const sampler)
    : PreRenderOperator(PostFXRenderStage::BLOOM, resolution, sampler),
      _outputFB(result),
      _tempHDRFB(nullptr),
      _luminaMipLevel(0)
{
    _luminaFB[0] = nullptr;
    _luminaFB[1] = nullptr;
    _horizBlur = 0;
    _vertBlur = 0;
    _tempBloomFB = GFX_DEVICE.newFB();

    TextureDescriptor tempBloomDescriptor(TextureType::TEXTURE_2D,
                                          GFXImageFormat::RGB8,
                                          GFXDataFormat::UNSIGNED_BYTE);
    tempBloomDescriptor.setSampler(*_internalSampler);

    _tempBloomFB->AddAttachment(tempBloomDescriptor, TextureDescriptor::AttachmentType::Color0);
    _outputFB->AddAttachment(tempBloomDescriptor, TextureDescriptor::AttachmentType::Color0);
    _outputFB->setClearColor(DefaultColors::BLACK());
    ResourceDescriptor bright("bright");
    bright.setThreadedLoading(false);
    ResourceDescriptor blur("blur");
    blur.setThreadedLoading(false);

    _bright = CreateResource<ShaderProgram>(bright);
    _blur = CreateResource<ShaderProgram>(blur);
    _bright->Uniform("texScreen", ShaderProgram::TextureUsage::UNIT0);
    _bright->Uniform("texExposure", ShaderProgram::TextureUsage::UNIT1);
    _bright->Uniform("texPrevExposure", 2);
    _blur->Uniform("texScreen", ShaderProgram::TextureUsage::UNIT0);
    _blur->Uniform("kernelSize", 10);
    _horizBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurHorizontal");
    _vertBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurVertical");
    reshape(_resolution.width, _resolution.height);
}

BloomPreRenderOperator::~BloomPreRenderOperator() {
    RemoveResource(_bright);
    RemoveResource(_blur);
    MemoryManager::DELETE(_tempBloomFB);
    MemoryManager::DELETE(_luminaFB[0]);
    MemoryManager::DELETE(_luminaFB[1]);
    MemoryManager::DELETE(_tempHDRFB);
}

U32 nextPOW2(U32 n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

void BloomPreRenderOperator::reshape(U16 width, U16 height) {
    assert(_tempBloomFB);
    U16 w = width / 4;
    U16 h = height / 4;
    _tempBloomFB->Create(w, h);
    _outputFB->Create(width, height);
    if (_genericFlag && _tempHDRFB) {
        _tempHDRFB->Create(width, height);
        U16 lumaRez = to_ushort(nextPOW2(width / 3));
        // make the texture square sized and power of two
        _luminaFB[0]->Create(lumaRez, lumaRez);
        _luminaFB[1]->Create(lumaRez, lumaRez);
        _luminaMipLevel = 0;
        while (lumaRez >>= 1) {
            _luminaMipLevel++;
        }
    }
    _blur->Uniform("size", vec2<F32>(w, h));
}

void BloomPreRenderOperator::operation() {
    if (!_enabled) return;

    if (_inputFB.empty()) {
        Console::errorfn(Locale::get("ERROR_BLOOM_INPUT_FB"));
        return;
    }

    size_t defaultStateHash = GFX_DEVICE.getDefaultStateBlock(true);

    _bright->bind();
    toneMapScreen();

    // render all of the "bright spots"
    _outputFB->Begin(Framebuffer::defaultPolicy());
    {
        // screen FB
        _inputFB[0]->Bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
        GFX_DEVICE.drawTriangle(defaultStateHash, _bright);
    }
    _outputFB->End();

    _blur->bind();
    // Blur horizontally
    _blur->SetSubroutine(ShaderType::FRAGMENT, _horizBlur);
    _tempBloomFB->Begin(Framebuffer::defaultPolicy());
    {
        // bright spots
        _outputFB->Bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
        GFX_DEVICE.drawTriangle(defaultStateHash, _blur);
    }
    _tempBloomFB->End();

    // Blur vertically
    _blur->SetSubroutine(ShaderType::FRAGMENT, _vertBlur);
    _outputFB->Begin(Framebuffer::defaultPolicy());
    {
        // horizontally blurred bright spots
        _tempBloomFB->Bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
        GFX_DEVICE.drawTriangle(defaultStateHash, _blur);
        // clear states
    }
    _outputFB->End();
}

void BloomPreRenderOperator::toneMapScreen() {
    if (!_genericFlag) return;

    if (!_tempHDRFB) {
        _tempHDRFB = GFX_DEVICE.newFB();
        TextureDescriptor hdrDescriptor(TextureType::TEXTURE_2D,
                                        GFXImageFormat::RGBA16F,
                                        GFXDataFormat::FLOAT_16);
        hdrDescriptor.setSampler(*_internalSampler);
        _tempHDRFB->AddAttachment(hdrDescriptor, TextureDescriptor::AttachmentType::Color0);
        _tempHDRFB->Create(_inputFB[0]->getWidth(), _inputFB[0]->getHeight());

        _luminaFB[0] = GFX_DEVICE.newFB();
        _luminaFB[1] = GFX_DEVICE.newFB();

        SamplerDescriptor lumaSampler;
        lumaSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
        lumaSampler.setMinFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);

        TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D,
                                         GFXImageFormat::RED16F,
                                         GFXDataFormat::FLOAT_16);
        lumaDescriptor.setSampler(lumaSampler);
        _luminaFB[0]->AddAttachment(lumaDescriptor, TextureDescriptor::AttachmentType::Color0);
        U16 lumaRez = to_ushort(nextPOW2(_inputFB[0]->getWidth() / 3));
        // make the texture square sized and power of two
        _luminaFB[0]->Create(lumaRez, lumaRez);

        lumaSampler.setFilters(TextureFilter::LINEAR);
        lumaDescriptor.setSampler(lumaSampler);
        _luminaFB[1]->AddAttachment(lumaDescriptor, TextureDescriptor::AttachmentType::Color0);

        _luminaFB[1]->Create(1, 1);
        _luminaMipLevel = 0;
        while (lumaRez >>= 1) _luminaMipLevel++;
    }

    _bright->Uniform("luminancePass", true);

    _luminaFB[1]->BlitFrom(_luminaFB[0]);

    _luminaFB[0]->Begin(Framebuffer::defaultPolicy());
    _inputFB[0]->Bind(0);
    _luminaFB[1]->Bind(2);
    GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _bright);

    _bright->Uniform("luminancePass", false);
    _bright->Uniform("toneMap", true);

    _tempHDRFB->BlitFrom(_inputFB[0]);

    _inputFB[0]->Begin(Framebuffer::defaultPolicy());
    // screen FB
    _tempHDRFB->Bind(0);
    // luminance FB
    _luminaFB[0]->Bind(1);
    GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _bright);
    _bright->Uniform("toneMap", false);
}
};