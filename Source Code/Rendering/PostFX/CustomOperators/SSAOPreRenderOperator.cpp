#include "Headers/SSAOPreRenderOperator.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

//ref: http://john-chapman-graphics.blogspot.co.uk/2013/01/ssao-tutorial.html
SSAOPreRenderOperator::SSAOPreRenderOperator(Framebuffer* hdrTarget, Framebuffer* ldrTarget)
    : PreRenderOperator(FilterType::FILTER_SS_AMBIENT_OCCLUSION, hdrTarget, ldrTarget)
{

    _samplerCopy = GFX_DEVICE.newFB();
    _samplerCopy->addAttachment(_hdrTarget->getDescriptor(), TextureDescriptor::AttachmentType::Color0);
    _samplerCopy->useAutoDepthBuffer(false);

    U16 ssaoNoiseSize = 4;
    U16 noiseDataSize = ssaoNoiseSize * ssaoNoiseSize;
    vectorImpl<vec3<F32>> noiseData(noiseDataSize);

    for (vec3<F32>& noise : noiseData) {
        noise.set(Random(-1.0f, 1.0f),
                  Random(-1.0f, 1.0f),
                  0.0f);
        noise.normalize();
    }

    U16 kernelSize = 32;
    vectorImpl<vec3<F32>> kernel(kernelSize);
    for (U16 i = 0; i < kernelSize; ++i) {
        vec3<F32>& k = kernel[i];
        k.set(Random(-1.0f, 1.0f),
              Random(-1.0f, 1.0f),
              Random( 0.0f, 1.0f));
        k.normalize();
        F32 scale = to_float(i) / to_float(kernelSize);
        k *= Lerp(0.1f, 1.0f, scale * scale);
    }
    
    SamplerDescriptor noiseSampler;
    noiseSampler.toggleMipMaps(false);
    noiseSampler.setAnisotropy(0);
    noiseSampler.setFilters(TextureFilter::NEAREST);
    noiseSampler.setWrapMode(TextureWrap::REPEAT);
    stringImpl attachmentName("SSAOPreRenderOperator_NoiseTexture");

    ResourceDescriptor textureAttachment(attachmentName);
    textureAttachment.setThreadedLoading(false);
    textureAttachment.setPropertyDescriptor(noiseSampler);
    textureAttachment.setEnumValue(to_uint(TextureType::TEXTURE_2D));
    _noiseTexture = CreateResource<Texture>(textureAttachment);

    TextureDescriptor noiseDescriptor;
    noiseDescriptor._internalFormat = GFXImageFormat::RGB16F;
    noiseDescriptor._samplerDescriptor = noiseSampler;
    noiseDescriptor._type = TextureType::TEXTURE_2D;
    _noiseTexture->loadData(Texture::TextureLoadInfo(),
                            noiseDescriptor,
                            noiseData.data(),
                            vec2<U16>(ssaoNoiseSize),
                            vec2<U16>(0, 1));

    SamplerDescriptor screenSampler;
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.setFilters(TextureFilter::LINEAR);
    screenSampler.toggleMipMaps(false);
    screenSampler.setAnisotropy(0);

    _ssaoOutput = GFX_DEVICE.newFB();
    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RED16,
                                       GFXDataFormat::FLOAT_16);
    outputDescriptor.setSampler(screenSampler);
    
    //Color0 holds the AO texture
    _ssaoOutput->addAttachment(outputDescriptor, TextureDescriptor::AttachmentType::Color0);

    _ssaoOutputBlurred = GFX_DEVICE.newFB();
    _ssaoOutputBlurred->addAttachment(outputDescriptor, TextureDescriptor::AttachmentType::Color0);

    ResourceDescriptor ssaoGenerate("SSAOPass.SSAOCalc");
    ssaoGenerate.setThreadedLoading(false);
    ssaoGenerate.setPropertyList(Util::StringFormat("USE_SCENE_ZPLANES,KERNEL_SIZE %d", kernelSize));
    _ssaoGenerateShader = CreateResource<ShaderProgram>(ssaoGenerate);

    ResourceDescriptor ssaoBlur("SSAOPass.SSAOBlur");
    ssaoBlur.setThreadedLoading(false);
    ssaoBlur.setPropertyList(Util::StringFormat("BLUR_SIZE %d", ssaoNoiseSize));
    _ssaoBlurShader = CreateResource<ShaderProgram>(ssaoBlur);
    
    ResourceDescriptor ssaoApply("SSAOPass.SSAOApply");
    ssaoApply.setThreadedLoading(false);
    _ssaoApplyShader = CreateResource<ShaderProgram>(ssaoApply);

    _ssaoGenerateShader->Uniform("sampleKernel", kernel);
}

SSAOPreRenderOperator::~SSAOPreRenderOperator() 
{
    MemoryManager::DELETE(_ssaoOutput);
    RemoveResource(_ssaoGenerateShader);
    RemoveResource(_ssaoBlurShader);
    RemoveResource(_noiseTexture);
}

void SSAOPreRenderOperator::idle() {
}

void SSAOPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    _ssaoOutput->create(width, height);
    _ssaoOutputBlurred->create(width, height);

    _ssaoGenerateShader->Uniform("noiseScale", vec2<F32>(width, height) / to_float(_noiseTexture->getWidth()));
    _ssaoBlurShader->Uniform("ssaoTexelSize", vec2<F32>(1.0f / _ssaoOutput->getWidth(),
                                                        1.0f / _ssaoOutput->getHeight()));
}

void SSAOPreRenderOperator::execute() {

    const Framebuffer::FramebufferDisplaySettings& settings = _hdrTarget->displaySettings();

    _ssaoGenerateShader->Uniform("projectionMatrix", settings._projectionMatrix);
    _ssaoGenerateShader->Uniform("invProjectionMatrix", settings._projectionMatrix.getInverse());

    _noiseTexture->Bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0)); // noise texture
    _inputFB[0]->bind(to_ubyte(ShaderProgram::TextureUsage::DEPTH),
                      TextureDescriptor::AttachmentType::Depth);  // depth
    _inputFB[0]->bind(to_ubyte(ShaderProgram::TextureUsage::NORMALMAP),
                      TextureDescriptor::AttachmentType::Color1);  // normals
    
    _ssaoOutput->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _ssaoGenerateShader);
    _ssaoOutput->end();


    _ssaoOutput->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0),
                      TextureDescriptor::AttachmentType::Color0);  // AO texture
    _ssaoOutputBlurred->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _ssaoBlurShader);
    _ssaoOutputBlurred->end();
    
    _samplerCopy->blitFrom(_hdrTarget);
    _samplerCopy->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0),
                       TextureDescriptor::AttachmentType::Color0);  // screen
    _ssaoOutputBlurred->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT1),
                             TextureDescriptor::AttachmentType::Color0);  // AO texture

    _hdrTarget->begin(_screenOnlyDraw);
        GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _ssaoApplyShader);
    _hdrTarget->end();
    
}

void SSAOPreRenderOperator::debugPreview(U8 slot) const {
    _ssaoOutputBlurred->bind(slot);
}

};