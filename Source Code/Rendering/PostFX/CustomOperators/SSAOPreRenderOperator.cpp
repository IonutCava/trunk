#include "stdafx.h"

#include "Headers/SSAOPreRenderOperator.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

//ref: http://john-chapman-graphics.blogspot.co.uk/2013/01/ssao-tutorial.html
SSAOPreRenderOperator::SSAOPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache* cache)
    : PreRenderOperator(context, parent, FilterType::FILTER_SS_AMBIENT_OCCLUSION),
      _enabled(true)
{
    U16 ssaoNoiseSize = 4;
    U16 noiseDataSize = ssaoNoiseSize * ssaoNoiseSize;
    vectorSTD<vec3<F32>> noiseData(noiseDataSize);

    for (vec3<F32>& noise : noiseData) {
        noise.set(Random(-1.0f, 1.0f),
                  Random(-1.0f, 1.0f),
                  0.0f);
    }

    U16 kernelSize = 64;
    vectorSTD<vec3<F32>> kernel(kernelSize);
    for (U16 i = 0; i < kernelSize; ++i) {
        vec3<F32>& k = kernel[i];
        k.set(Random(-1.0f, 1.0f),
              Random(-1.0f, 1.0f),
              Random( 0.0f, 1.0f));
        k.normalize();
        F32 scaleSq = SQUARED(to_F32(i) / to_F32(kernelSize));
        k *= Lerp(0.1f, 1.0f, scaleSq);
    }
    
    SamplerDescriptor noiseSampler = {};
    noiseSampler.wrapU(TextureWrap::REPEAT);
    noiseSampler.wrapV(TextureWrap::REPEAT);
    noiseSampler.wrapW(TextureWrap::REPEAT);
    noiseSampler.minFilter(TextureFilter::NEAREST);
    noiseSampler.magFilter(TextureFilter::NEAREST);
    noiseSampler.anisotropyLevel(0);

    Str64 attachmentName("SSAOPreRenderOperator_NoiseTexture");

    TextureDescriptor noiseDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGB, GFXDataFormat::FLOAT_16);
    noiseDescriptor.samplerDescriptor(noiseSampler);

    ResourceDescriptor textureAttachment(attachmentName);
    textureAttachment.propertyDescriptor(noiseDescriptor);
    _noiseTexture = CreateResource<Texture>(cache, textureAttachment);

   
    _noiseTexture->loadData(Texture::TextureLoadInfo(),
                            noiseData.data(),
                            vec2<U16>(ssaoNoiseSize));

    SamplerDescriptor screenSampler = {};
    screenSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.minFilter(TextureFilter::LINEAR);
    screenSampler.magFilter(TextureFilter::LINEAR);
    screenSampler.anisotropyLevel(0);

    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RED, GFXDataFormat::FLOAT_16);
    outputDescriptor.samplerDescriptor(screenSampler);

    {
        vectorSTD<RTAttachmentDescriptor> att = {
            { outputDescriptor, RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "SSAO_Out";
        desc._resolution = vec2<U16>(parent.inputRT()._rt->getWidth(), parent.inputRT()._rt->getHeight());
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        //Colour0 holds the AO texture
        _ssaoOutput = _context.renderTargetPool().allocateRT(desc);
    }

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "FullScreenQuad";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "SSAOPass.glsl";
    fragModule._variant = "SSAOCalc";
    fragModule._defines.emplace_back(Util::StringFormat("KERNEL_SIZE %d", kernelSize).c_str(), true);

    ShaderProgramDescriptor ssaoShaderDescriptor = {};
    ssaoShaderDescriptor._modules.push_back(vertModule);
    ssaoShaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor ssaoGenerate("SSAOCalc");
    ssaoGenerate.propertyDescriptor(ssaoShaderDescriptor);
    ssaoGenerate.waitForReady(false);
    _ssaoGenerateShader = CreateResource<ShaderProgram>(cache, ssaoGenerate);

    fragModule._variant = "SSAOBlur";
    fragModule._defines.resize(0);
    fragModule._defines.emplace_back(Util::StringFormat("BLUR_SIZE %d", ssaoNoiseSize).c_str(), true);

    ssaoShaderDescriptor = {};
    ssaoShaderDescriptor._modules.push_back(vertModule);
    ssaoShaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor ssaoBlur("SSAOBlur");
    ssaoBlur.propertyDescriptor(ssaoShaderDescriptor);
    ssaoBlur.waitForReady(false);
    _ssaoBlurShader = CreateResource<ShaderProgram>(cache, ssaoBlur);
    
    _ssaoGenerateConstants.set(_ID("sampleKernel"), GFX::PushConstantType::VEC3, kernel);
    _ssaoBlurConstants.set(_ID("passThrough"), GFX::PushConstantType::BOOL, false);
    
    radius(context.context().config().rendering.postFX.ssaoRadius);
    power(context.context().config().rendering.postFX.ssaoPower);
}

SSAOPreRenderOperator::~SSAOPreRenderOperator() 
{
    _context.renderTargetPool().deallocateRT(_ssaoOutput);
}

void SSAOPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    _ssaoOutput._rt->resize(width, height);

    _ssaoGenerateConstants.set(_ID("noiseScale"), GFX::PushConstantType::VEC2, vec2<F32>(width, height) / to_F32(_noiseTexture->width()));
}

void SSAOPreRenderOperator::radius(const F32 val) {
    _radius = val;
    _ssaoGenerateConstants.set(_ID("radius"), GFX::PushConstantType::FLOAT, _radius);
    _context.context().config().rendering.postFX.ssaoRadius = val;
}

void SSAOPreRenderOperator::power(const F32 val) {
    _power = val;
    _ssaoGenerateConstants.set(_ID("power"), GFX::PushConstantType::FLOAT, _power);
    _context.context().config().rendering.postFX.ssaoPower = val;
}

void SSAOPreRenderOperator::prepare(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    if (_enabled) {
        _ssaoGenerateConstants.set(_ID("projectionMatrix"), GFX::PushConstantType::MAT4, camera.getProjectionMatrix());
        _ssaoGenerateConstants.set(_ID("invProjectionMatrix"), GFX::PushConstantType::MAT4, GetInverse(camera.getProjectionMatrix()));

        // Generate AO
        GFX::BeginRenderPassCommand beginRenderPassCmd;
        beginRenderPassCmd._target = _ssaoOutput._targetID;
        beginRenderPassCmd._name = "DO_SSAO_CALC";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        pipelineDescriptor._shaderProgramHandle = _ssaoGenerateShader->getGUID();
        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

        TextureData data = _noiseTexture->data();
        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set._textureData.setTexture(data, to_U8(TextureUsage::UNIT0));

        data = _parent.inputRT()._rt->getAttachment(RTAttachmentType::Depth, 0).texture()->data();
        descriptorSetCmd._set._textureData.setTexture(data, to_U8(TextureUsage::DEPTH));

        data = _parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY)).texture()->data();
        descriptorSetCmd._set._textureData.setTexture(data, to_U8(TextureUsage::NORMALMAP));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);


        GFX::EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _ssaoGenerateConstants });

        GFX::EnqueueCommand(bufferInOut, GFX::DrawCommand{ triangleCmd });

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
    }

    // Blur AO
    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._descriptor.drawMask().disableAll();
    beginRenderPassCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::EXTRA), true);
    beginRenderPassCmd._target = _parent.inputRT()._targetID;
    beginRenderPassCmd._name = "DO_SSAO_BLUR";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    pipelineDescriptor._shaderProgramHandle = _ssaoBlurShader->getGUID();
    GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

    TextureData data = _ssaoOutput._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data();  // AO texture
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.setTexture(data, to_U8(TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _ssaoBlurConstants });


    GFX::EnqueueCommand(bufferInOut, GFX::DrawCommand{ triangleCmd });

    GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
}

void SSAOPreRenderOperator::execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(camera);
    ACKNOWLEDGE_UNUSED(bufferInOut);
}

void SSAOPreRenderOperator::onToggle(const bool state) {
    PreRenderOperator::onToggle(state);
    _enabled = state;
    _ssaoBlurConstants.set(_ID("passThrough"), GFX::PushConstantType::BOOL, !state);
}
};