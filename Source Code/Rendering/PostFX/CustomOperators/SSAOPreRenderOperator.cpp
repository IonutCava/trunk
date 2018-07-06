#include "stdafx.h"

#include "Headers/SSAOPreRenderOperator.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

//ref: http://john-chapman-graphics.blogspot.co.uk/2013/01/ssao-tutorial.html
SSAOPreRenderOperator::SSAOPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_SS_AMBIENT_OCCLUSION)
{
    vec2<U16> res(parent.inputRT()._rt->getWidth(), parent.inputRT()._rt->getHeight());

    {
        vectorImpl<RTAttachmentDescriptor> att = {
            { parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getDescriptor(), RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "SSAO";
        desc._resolution = res;
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _samplerCopy = _context.renderTargetPool().allocateRT(desc);
    }

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
        F32 scale = to_F32(i) / to_F32(kernelSize);
        k *= Lerp(0.1f, 1.0f, scale * scale);
    }
    
    SamplerDescriptor noiseSampler;
    noiseSampler.setAnisotropy(0);
    noiseSampler.setFilters(TextureFilter::NEAREST);
    noiseSampler.setWrapMode(TextureWrap::REPEAT);
    noiseSampler.toggleSRGBColourSpace(true);
    stringImpl attachmentName("SSAOPreRenderOperator_NoiseTexture");

    TextureDescriptor noiseDescriptor(TextureType::TEXTURE_2D,
                                      GFXImageFormat::RGB16F);
    noiseDescriptor.setSampler(noiseSampler);

    ResourceDescriptor textureAttachment(attachmentName);
    textureAttachment.setThreadedLoading(false);
    textureAttachment.setPropertyDescriptor(noiseDescriptor);
    _noiseTexture = CreateResource<Texture>(cache, textureAttachment);

   
    _noiseTexture->loadData(Texture::TextureLoadInfo(),
                            noiseData.data(),
                            vec2<U16>(ssaoNoiseSize));

    SamplerDescriptor screenSampler;
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.setFilters(TextureFilter::LINEAR);
    screenSampler.setAnisotropy(0);

    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RED16,
                                       GFXDataFormat::FLOAT_16);
    outputDescriptor.setSampler(screenSampler);

    {
        vectorImpl<RTAttachmentDescriptor> att = {
            { outputDescriptor, RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "SSAO_Out";
        desc._resolution = res;
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        //Colour0 holds the AO texture
        _ssaoOutput = _context.renderTargetPool().allocateRT(desc);

        desc._name = "SSAO_Blurred_Out";
        _ssaoOutputBlurred = _context.renderTargetPool().allocateRT(desc);
    }
    ResourceDescriptor ssaoGenerate("SSAOPass.SSAOCalc");
    ssaoGenerate.setThreadedLoading(false);
    ssaoGenerate.setPropertyList(Util::StringFormat("USE_SCENE_ZPLANES,KERNEL_SIZE %d", kernelSize));
    _ssaoGenerateShader = CreateResource<ShaderProgram>(cache, ssaoGenerate);

    ResourceDescriptor ssaoBlur("SSAOPass.SSAOBlur");
    ssaoBlur.setThreadedLoading(false);
    ssaoBlur.setPropertyList(Util::StringFormat("BLUR_SIZE %d", ssaoNoiseSize));
    _ssaoBlurShader = CreateResource<ShaderProgram>(cache, ssaoBlur);
    
    ResourceDescriptor ssaoApply("SSAOPass.SSAOApply");
    ssaoApply.setThreadedLoading(false);
    _ssaoApplyShader = CreateResource<ShaderProgram>(cache, ssaoApply);

    _ssaoGenerateConstants.set("sampleKernel", GFX::PushConstantType::VEC3, kernel);
}

SSAOPreRenderOperator::~SSAOPreRenderOperator() 
{
    _context.renderTargetPool().deallocateRT(_ssaoOutput);
    _context.renderTargetPool().deallocateRT(_ssaoOutputBlurred);
}

void SSAOPreRenderOperator::idle(const Configuration& config) {
    ACKNOWLEDGE_UNUSED(config);
}

void SSAOPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    _ssaoOutput._rt->resize(width, height);
    _ssaoOutputBlurred._rt->resize(width, height);

    _ssaoGenerateConstants.set("noiseScale", GFX::PushConstantType::VEC2, vec2<F32>(width, height) / to_F32(_noiseTexture->getWidth()));

    _ssaoBlurConstants.set("ssaoTexelSize", GFX::PushConstantType::VEC2, vec2<F32>(1.0f / _ssaoOutput._rt->getWidth(),
                                                                                  1.0f / _ssaoOutput._rt->getHeight()));
}

void SSAOPreRenderOperator::execute(GFX::CommandBuffer& bufferInOut) {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);

    _ssaoGenerateConstants.set("projectionMatrix", GFX::PushConstantType::MAT4, PreRenderOperator::s_mainCamProjectionMatrixCache);
    _ssaoGenerateConstants.set("invProjectionMatrix", GFX::PushConstantType::MAT4, PreRenderOperator::s_mainCamProjectionMatrixCache.getInverse());

    RenderTargetHandle screen = _parent.inputRT();

    // Generate AO
    pipelineDescriptor._shaderProgram = _ssaoGenerateShader;
    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = &_context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    TextureData data = _noiseTexture->getData();
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));

    data = screen._rt->getAttachment(RTAttachmentType::Depth, 0).texture()->getData();
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::DEPTH));

    data = screen._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS)).texture()->getData();
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::NORMALMAP));
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _ssaoGenerateConstants;
    GFX::SendPushConstants(bufferInOut, pushConstantsCommand);

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = _ssaoOutput._targetID;
    beginRenderPassCmd._name = "DO_SSAO_PASS";
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);

    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(triangleCmd);
    GFX::AddDrawCommands(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);

    // Blur AO
    pipelineDescriptor._shaderProgram = _ssaoBlurShader;
    pipelineCmd._pipeline = &_context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    descriptorSetCmd._set._textureData.clear();
    data = _ssaoOutput._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();  // AO texture
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    pushConstantsCommand._constants = _ssaoBlurConstants;
    GFX::SendPushConstants(bufferInOut, pushConstantsCommand);

    beginRenderPassCmd._target = _ssaoOutputBlurred._targetID;
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);

    GFX::AddDrawCommands(bufferInOut, drawCmd);

    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);
    
    // Apply AO
    _samplerCopy._rt->blitFrom(screen._rt);
    GFX::BlitRenderTargetCommand blitRTCommand;
    blitRTCommand._source = screen._targetID;
    blitRTCommand._destination = _samplerCopy._targetID;
    GFX::BlitRenderTarget(bufferInOut, blitRTCommand);

    pipelineDescriptor._shaderProgram = _ssaoApplyShader;
    pipelineCmd._pipeline = &_context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    descriptorSetCmd._set._textureData.clear();
    data = _samplerCopy._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();  // screen
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));

    data = _ssaoOutputBlurred._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();  // AO texture
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT1));
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    pipelineDescriptor._shaderProgram = _ssaoApplyShader;
    pipelineCmd._pipeline = &_context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    beginRenderPassCmd._target = screen._targetID;
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);

    GFX::AddDrawCommands(bufferInOut, drawCmd);

    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);
}

TextureData SSAOPreRenderOperator::getDebugOutput() const {
    TextureData ret;
    ret = _ssaoOutputBlurred._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    return ret;
}

};