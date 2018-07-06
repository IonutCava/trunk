#include "Headers/SSAOPreRenderOperator.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

//ref: http://john-chapman-graphics.blogspot.co.uk/2013/01/ssao-tutorial.html
SSAOPreRenderOperator::SSAOPreRenderOperator(Framebuffer* renderTarget)
    : PreRenderOperator(FilterType::FILTER_SS_AMBIENT_OCCLUSION, renderTarget)
{
    U16 ssaoNoiseSize = 4;
    U16 noiseDataSize = ssaoNoiseSize * ssaoNoiseSize;
    vectorImpl<vec3<F32>> noiseData(noiseDataSize);

    for (vec3<F32>& noise : noiseData) {
        noise.set(Random(-1.0f, 1.0f),
                  Random(-1.0f, 1.0f),
                  0.0f);
        noise.normalize();
    }

    SamplerDescriptor noiseSampler;
    noiseSampler.toggleMipMaps(false);
    noiseSampler.setAnisotropy(0);
    noiseSampler.setWrapMode(TextureWrap::CLAMP);
    stringImpl attachmentName("SSAOPreRenderOperator_NoiseTexture");
    ResourceDescriptor textureAttachment(attachmentName);
    textureAttachment.setThreadedLoading(false);
    textureAttachment.setPropertyDescriptor(noiseSampler);
    textureAttachment.setEnumValue(to_uint(TextureType::TEXTURE_2D));
    _noiseTexture = CreateResource<Texture>(textureAttachment);
    TextureDescriptor noiseDescriptor;
    noiseDescriptor._internalFormat = GFXImageFormat::RGB32F;
    noiseDescriptor._samplerDescriptor = noiseSampler;
    noiseDescriptor._type = TextureType::TEXTURE_2D;
    _noiseTexture->loadData(Texture::TextureLoadInfo(), noiseDescriptor, noiseData.data(), vec2<U16>(ssaoNoiseSize), vec2<U16>(0, 1));

    SamplerDescriptor screenSampler;
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.setFilters(TextureFilter::NEAREST);
    screenSampler.toggleMipMaps(false);
    screenSampler.setAnisotropy(0);

    _ssaoOutput = GFX_DEVICE.newFB();
    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RGB8,
                                       GFXDataFormat::UNSIGNED_BYTE);
    outputDescriptor.setSampler(screenSampler);
    
    //Color0 holds the AO texture
    _ssaoOutput->addAttachment(outputDescriptor, TextureDescriptor::AttachmentType::Color0);
    //Color1 holds a copy of the screen
    TextureDescriptor screenCopyDescriptor(_renderTarget->getAttachment(TextureDescriptor::AttachmentType::Color0)->getDescriptor());
    _ssaoOutput->addAttachment(screenCopyDescriptor, TextureDescriptor::AttachmentType::Color1);

    _ssaoOutput->create(renderTarget->getWidth(), renderTarget->getHeight());

    ResourceDescriptor ssaoGenerate("SSAOPass");
    ssaoGenerate.setThreadedLoading(false);
    _ssaoGenerateShader = CreateResource<ShaderProgram>(ssaoGenerate);

    ResourceDescriptor ssaoApply("SSAOApply");
    ssaoApply.setThreadedLoading(false);
    _ssaoApplyShader = CreateResource<ShaderProgram>(ssaoApply);
}

SSAOPreRenderOperator::~SSAOPreRenderOperator() 
{
    MemoryManager::DELETE(_ssaoOutput);
    RemoveResource(_ssaoGenerateShader);
    RemoveResource(_ssaoApplyShader);
    RemoveResource(_noiseTexture);
}

void SSAOPreRenderOperator::idle() {
}

void SSAOPreRenderOperator::reshape(U16 width, U16 height) {
    _ssaoOutput->create(width, height);
}

void SSAOPreRenderOperator::execute() {
    STUBBED("SSAO implementation is incomplete! -Ionut");
    return;

    _renderTarget->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0),
                        TextureDescriptor::AttachmentType::Color0);  // screen
    _inputFB[0]->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT1),
                      TextureDescriptor::AttachmentType::Depth);  // depth

    _ssaoOutput->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _ssaoGenerateShader);
    _ssaoOutput->end();


    _ssaoOutput->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0),
                      TextureDescriptor::AttachmentType::Color1);  // screen
    _ssaoOutput->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT1),
                      TextureDescriptor::AttachmentType::Color0);  // AO texture

    _renderTarget->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _ssaoApplyShader);
    _renderTarget->end();
}

void SSAOPreRenderOperator::debugPreview(U8 slot) const {
    _ssaoOutput->bind(slot);
}

};