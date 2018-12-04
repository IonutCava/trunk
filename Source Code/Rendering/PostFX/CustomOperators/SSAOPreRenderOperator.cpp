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
        vector<RTAttachmentDescriptor> att = {
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
    vector<vec3<F32>> noiseData(noiseDataSize);

    for (vec3<F32>& noise : noiseData) {
        noise.set(Random(-1.0f, 1.0f),
                  Random(-1.0f, 1.0f),
                  0.0f);
        noise.normalize();
    }

    U16 kernelSize = 32;
    vector<vec3<F32>> kernel(kernelSize);
    for (U16 i = 0; i < kernelSize; ++i) {
        vec3<F32>& k = kernel[i];
        k.set(Random(-1.0f, 1.0f),
              Random(-1.0f, 1.0f),
              Random( 0.0f, 1.0f));
        k.normalize();
        F32 scale = to_F32(i) / to_F32(kernelSize);
        k *= Lerp(0.1f, 1.0f, scale * scale);
    }
    
    SamplerDescriptor noiseSampler = {};
    noiseSampler._wrapU = TextureWrap::REPEAT;
    noiseSampler._wrapV = TextureWrap::REPEAT;
    noiseSampler._wrapW = TextureWrap::REPEAT;
    noiseSampler._minFilter = TextureFilter::NEAREST;
    noiseSampler._magFilter = TextureFilter::NEAREST;
    noiseSampler._anisotropyLevel = 0;

    stringImpl attachmentName("SSAOPreRenderOperator_NoiseTexture");

    TextureDescriptor noiseDescriptor(TextureType::TEXTURE_2D,
                                      GFXImageFormat::RGB16F);
    noiseDescriptor.setSampler(noiseSampler);
    noiseDescriptor._srgb = true;

    ResourceDescriptor textureAttachment(attachmentName);
    textureAttachment.setThreadedLoading(false);
    textureAttachment.setPropertyDescriptor(noiseDescriptor);
    _noiseTexture = CreateResource<Texture>(cache, textureAttachment);

   
    _noiseTexture->loadData(Texture::TextureLoadInfo(),
                            noiseData.data(),
                            vec2<U16>(ssaoNoiseSize));

    SamplerDescriptor screenSampler = {};
    screenSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    screenSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    screenSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    screenSampler._minFilter = TextureFilter::LINEAR;
    screenSampler._magFilter = TextureFilter::LINEAR;
    screenSampler._anisotropyLevel = 0;

    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RED16);
    outputDescriptor.setSampler(screenSampler);

    {
        vector<RTAttachmentDescriptor> att = {
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

    ShaderProgramDescriptor ssaoGenerateDescriptor;
    ssaoGenerateDescriptor._defines.push_back(std::make_pair(Util::StringFormat("KERNEL_SIZE %d", kernelSize), true));
    ssaoGenerate.setPropertyDescriptor(ssaoGenerateDescriptor);
    _ssaoGenerateShader = CreateResource<ShaderProgram>(cache, ssaoGenerate);

    ResourceDescriptor ssaoBlur("SSAOPass.SSAOBlur");
    ssaoBlur.setThreadedLoading(false);
    ShaderProgramDescriptor ssaoBlurDescriptor;
    ssaoBlurDescriptor._defines.push_back(std::make_pair(Util::StringFormat("BLUR_SIZE %d", ssaoNoiseSize), true));
    ssaoBlur.setPropertyDescriptor(ssaoBlurDescriptor);

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

void SSAOPreRenderOperator::execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;

    _ssaoGenerateConstants.set("projectionMatrix", GFX::PushConstantType::MAT4, camera.getProjectionMatrix());
    _ssaoGenerateConstants.set("invProjectionMatrix", GFX::PushConstantType::MAT4, camera.getProjectionMatrix().getInverse());

    RenderTargetHandle screen = _parent.inputRT();

    // Generate AO
    pipelineDescriptor._shaderProgramHandle = _ssaoGenerateShader->getID();
    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    TextureData data = _noiseTexture->getData();
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set = _context.newDescriptorSet();
    descriptorSetCmd._set->_textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));

    data = screen._rt->getAttachment(RTAttachmentType::Depth, 0).texture()->getData();
    descriptorSetCmd._set->_textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::DEPTH));

    data = screen._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS)).texture()->getData();
    descriptorSetCmd._set->_textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::NORMALMAP));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _ssaoGenerateConstants;
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = _ssaoOutput._targetID;
    beginRenderPassCmd._name = "DO_SSAO_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(triangleCmd);
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

    // Blur AO
    pipelineDescriptor._shaderProgramHandle = _ssaoBlurShader->getID();
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    descriptorSetCmd._set = _context.newDescriptorSet();
    data = _ssaoOutput._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();  // AO texture
    descriptorSetCmd._set->_textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    pushConstantsCommand._constants = _ssaoBlurConstants;
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

    beginRenderPassCmd._target = _ssaoOutputBlurred._targetID;
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
    
    // Apply AO
    GFX::BlitRenderTargetCommand blitRTCommand;
    blitRTCommand._source = screen._targetID;
    blitRTCommand._destination = _samplerCopy._targetID;
    blitRTCommand._blitColours.emplace_back();
    GFX::EnqueueCommand(bufferInOut, blitRTCommand);

    pipelineDescriptor._shaderProgramHandle = _ssaoApplyShader->getID();
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    descriptorSetCmd._set = _context.newDescriptorSet();
    data = _samplerCopy._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();  // screen
    descriptorSetCmd._set->_textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));

    data = _ssaoOutputBlurred._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();  // AO texture
    descriptorSetCmd._set->_textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT1));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    pipelineDescriptor._shaderProgramHandle = _ssaoApplyShader->getID();
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    beginRenderPassCmd._target = screen._targetID;
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

TextureData SSAOPreRenderOperator::getDebugOutput() const {
    TextureData ret;
    ret = _ssaoOutputBlurred._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    return ret;
}

};