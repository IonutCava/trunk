#include "stdafx.h"

#include "Headers/PostAAPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Core/Headers/Configuration.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

PostAAPreRenderOperator::PostAAPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_SS_ANTIALIASING)
{
    useSMAA(_ID(cache.context().config().rendering.postFX.postAAType.c_str()) == _ID("SMAA"));
    postAAQualityLevel(cache.context().config().rendering.postFX.PostAAQualityLevel);

    RenderTargetDescriptor desc = {};
    desc._resolution = parent.inputRT()._rt->getResolution();

    {
        vectorSTD<RTAttachmentDescriptor> att = {
            { parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->descriptor(), RTAttachmentType::Colour },
        };

        desc._name = "PostAA";
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _samplerCopy = _context.renderTargetPool().allocateRT(desc);
    }

    {
        SamplerDescriptor sampler = {};
        sampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
        sampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
        sampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
        sampler.minFilter(TextureFilter::LINEAR);
        sampler.magFilter(TextureFilter::LINEAR);
        sampler.anisotropyLevel(0);

        TextureDescriptor weightsDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGBA, GFXDataFormat::FLOAT_16);
        weightsDescriptor.samplerDescriptor(sampler);

        vectorSTD<RTAttachmentDescriptor> att = {
            { weightsDescriptor, RTAttachmentType::Colour }
        };

        desc._name = "SMAAWeights";
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _smaaWeights = _context.renderTargetPool().allocateRT(desc);
    }

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "Dummy";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "FXAA.glsl";

    ShaderProgramDescriptor aaShaderDescriptor = {};
    aaShaderDescriptor._modules = { vertModule, fragModule };

    ResourceDescriptor fxaa("FXAA");
    fxaa.propertyDescriptor(aaShaderDescriptor);
    fxaa.waitForReady(false);
    _fxaa = CreateResource<ShaderProgram>(cache, fxaa);

    _pushConstantsCommand._constants.set(_ID("dvd_qualityMultiplier"), GFX::PushConstantType::INT, to_I32(postAAQualityLevel()));

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _fxaa->getGUID();
    _fxaaPipeline = _context.newPipeline(pipelineDescriptor);
}

void PostAAPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
    _smaaWeights._rt->resize(width, height);
}

void PostAAPreRenderOperator::prepare(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(camera);
    ACKNOWLEDGE_UNUSED(bufferInOut);

    if (useSMAA() != currentUseSMAA()) {
        currentUseSMAA(useSMAA());

        _context.context().config().rendering.postFX.postAAType = useSMAA() ? "SMAA" : "FXAA";
        _context.context().config().changed(true);
    }

    if (postAAQualityLevel() != currentPostAAQualityLevel()) {
        currentPostAAQualityLevel(postAAQualityLevel());

        _pushConstantsCommand._constants.set(_ID("dvd_qualityMultiplier"), GFX::PushConstantType::INT, to_I32(postAAQualityLevel()));

        _context.context().config().rendering.postFX.PostAAQualityLevel = postAAQualityLevel();
        _context.context().config().changed(true);
    }
}

/// This is tricky as we use our screen as both input and output
void PostAAPreRenderOperator::execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    STUBBED("ToDo: Move PostAA to compute shaders to avoid a blit and RT swap. -Ionut");
    if (useSMAA()) {
    } else {
        GFX::BlitRenderTargetCommand blitRTCommand = {};
        blitRTCommand._source = _parent.outputRT()._targetID;
        blitRTCommand._destination = _samplerCopy._targetID;
        blitRTCommand._blitColours.emplace_back();
        GFX::EnqueueCommand(bufferInOut, blitRTCommand);

        const TextureData data0 = _samplerCopy._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data();

        // Apply FXAA/SMAA to the specified render target
        GFX::BeginRenderPassCommand beginRenderPassCmd;
        beginRenderPassCmd._target = _parent.outputRT()._targetID;
        beginRenderPassCmd._name = "DO_POSTAA_PASS";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _fxaaPipeline });
        GFX::EnqueueCommand(bufferInOut, _pushConstantsCommand);

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::EnqueueCommand(bufferInOut, _pointDrawCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{ });
    }
}

};