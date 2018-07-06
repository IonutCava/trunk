#include "Headers/BloomPreRenderOperator.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/PostFX/Headers/PreRenderStageBuilder.h"

BloomPreRenderOperator::BloomPreRenderOperator(Framebuffer* result,
                                               const vec2<U16>& resolution,
                                               SamplerDescriptor* const sampler) : PreRenderOperator(BLOOM_STAGE,resolution,sampler),
                                                                                   _outputFB(result),
                                                                                   _tempHDRFB(nullptr),
                                                                                   _luminaMipLevel(0)
{
    _luminaFB[0] = nullptr;
    _luminaFB[1] = nullptr;
    F32 width = _resolution.width;
    F32 height = _resolution.height;
    _horizBlur = 0;
    _vertBlur = 0;
    _tempBloomFB = GFX_DEVICE.newFB();

    TextureDescriptor tempBloomDescriptor(TEXTURE_2D, RGB8, UNSIGNED_BYTE);
    tempBloomDescriptor.setSampler(*_internalSampler);

    _tempBloomFB->AddAttachment(tempBloomDescriptor,TextureDescriptor::Color0);
    _outputFB->AddAttachment(tempBloomDescriptor, TextureDescriptor::Color0);
    ResourceDescriptor bright("bright");
    bright.setThreadedLoading(false);
    ResourceDescriptor blur("blur");
    blur.setThreadedLoading(false);

    _bright = CreateResource<ShaderProgram>(bright);
    _blur = CreateResource<ShaderProgram>(blur);
    _bright->UniformTexture("texScreen", 0);
    _bright->UniformTexture("texExposure", 1);
    _bright->UniformTexture("texPrevExposure", 2);
    _blur->UniformTexture("texScreen", 0);
    _blur->Uniform("kernelSize", 10);
    _horizBlur = _blur->GetSubroutineIndex(FRAGMENT_SHADER, "blurHorizontal");
    _vertBlur  = _blur->GetSubroutineIndex(FRAGMENT_SHADER, "blurVertical");
    reshape(width, height);
}

BloomPreRenderOperator::~BloomPreRenderOperator(){
    RemoveResource(_bright);
    RemoveResource(_blur);
    SAFE_DELETE(_tempBloomFB);
    SAFE_DELETE(_luminaFB[0]);
    SAFE_DELETE(_luminaFB[1]);
    SAFE_DELETE(_tempHDRFB);
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

void BloomPreRenderOperator::reshape(I32 width, I32 height){
    assert(_tempBloomFB);
    I32 w = width / 4;
    I32 h = height / 4;
    _tempBloomFB->Create(w,h);
    _outputFB->Create(width, height);
    if(_genericFlag && _tempHDRFB){
        _tempHDRFB->Create(width, height);
        U32 lumaRez = nextPOW2(width / 3);
        // make the texture square sized and power of two
        _luminaFB[0]->Create(lumaRez , lumaRez);
        _luminaFB[1]->Create(lumaRez , lumaRez);
        _luminaMipLevel = 0;
        while(lumaRez>>=1) _luminaMipLevel++;
    }
    _blur->Uniform("size", vec2<F32>((F32)w, (F32)h));
}

void BloomPreRenderOperator::operation(){
    if(!_enabled)
        return;

    if(_inputFB.empty()){
        ERROR_FN(Locale::get("ERROR_BLOOM_INPUT_FB"));
        return; 
    }

    _bright->bind();
    toneMapScreen();
    _bright->Uniform("toneMap", false);
    // render all of the "bright spots"
    _outputFB->Begin(Framebuffer::defaultPolicy());
    //screen FB
    _inputFB[0]->Bind(0);
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true));
    _blur->bind();
    _blur->SetSubroutine(FRAGMENT_SHADER, _horizBlur);
    _outputFB->End();
    //Blur horizontally
    _tempBloomFB->Begin(Framebuffer::defaultPolicy());
    //bright spots
    _outputFB->Bind(0);
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true));
    _blur->SetSubroutine(FRAGMENT_SHADER, _vertBlur);
    _tempBloomFB->End();
    //Blur vertically
    _outputFB->Begin(Framebuffer::defaultPolicy());
    //horizontally blurred bright spots
    _tempBloomFB->Bind(0);
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true));
    // clear states
    _outputFB->End();
}

void BloomPreRenderOperator::toneMapScreen()
{
    if(!_genericFlag)
        return;

    if(!_tempHDRFB){
        _tempHDRFB = GFX_DEVICE.newFB();
        TextureDescriptor hdrDescriptor(TEXTURE_2D, RGBA16F, FLOAT_16);
        hdrDescriptor.setSampler(*_internalSampler);
        _tempHDRFB->AddAttachment(hdrDescriptor, TextureDescriptor::Color0);
        _tempHDRFB->Create(_inputFB[0]->getWidth(), _inputFB[0]->getHeight());

        _luminaFB[0] = GFX_DEVICE.newFB();
        _luminaFB[1] = GFX_DEVICE.newFB();

        SamplerDescriptor lumaSampler;
        lumaSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
        lumaSampler.setMinFilter(TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR);
        lumaSampler.toggleMipMaps(true);

        TextureDescriptor lumaDescriptor(TEXTURE_2D, RED16F, FLOAT_16);
        lumaDescriptor.setSampler(lumaSampler);
        _luminaFB[0]->AddAttachment(lumaDescriptor, TextureDescriptor::Color0);
        U32 lumaRez = nextPOW2(_inputFB[0]->getWidth() / 3);
        // make the texture square sized and power of two
        _luminaFB[0]->Create(lumaRez , lumaRez);

        lumaSampler.setFilters(TEXTURE_FILTER_LINEAR);
        lumaSampler.toggleMipMaps(false);
        lumaDescriptor.setSampler(lumaSampler);
        _luminaFB[1]->AddAttachment(lumaDescriptor, TextureDescriptor::Color0);

        _luminaFB[1]->Create(1, 1);
        _luminaMipLevel = 0;
        while(lumaRez>>=1) _luminaMipLevel++;
    }

    _bright->Uniform("luminancePass", true);

    _luminaFB[1]->BlitFrom(_luminaFB[0]);

    _luminaFB[0]->Begin(Framebuffer::defaultPolicy());
    _inputFB[0]->Bind(0);
    _luminaFB[1]->Bind(2);
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true));
    
    _bright->Uniform("luminancePass", false);
    _bright->Uniform("toneMap", true);

    _tempHDRFB->BlitFrom(_inputFB[0]);

    _inputFB[0]->Begin(Framebuffer::defaultPolicy());
    //screen FB
    _tempHDRFB->Bind(0);
    // luminance FB
    _luminaFB[0]->Bind(1);
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true));
}