#include "stdafx.h"

#include "Headers/PostAAPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Core/Headers/Configuration.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

PostAAPreRenderOperator::PostAAPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_SS_ANTIALIASING),
      _useSMAA(false),
      _postAASamples(0),
      _idleCount(0)
{
    vectorImpl<RTAttachmentDescriptor> att = {
        { parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getDescriptor(), RTAttachmentType::Colour },
    };

    RenderTargetDescriptor desc = {};
    desc._name = "PostAA";
    desc._resolution = vec2<U16>(parent.inputRT()._rt->getWidth(), parent.inputRT()._rt->getHeight());
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    _samplerCopy = _context.renderTargetPool().allocateRT(desc);

    
    ResourceDescriptor fxaa("FXAA");
    fxaa.setThreadedLoading(false);
    _fxaa = CreateResource<ShaderProgram>(cache, fxaa);

    ResourceDescriptor smaa("SMAA");
    smaa.setThreadedLoading(false);
    _smaa = CreateResource<ShaderProgram>(cache, smaa);
}

PostAAPreRenderOperator::~PostAAPreRenderOperator()
{
}

void PostAAPreRenderOperator::idle(const Configuration& config) {
    I32 samples = config.rendering.postAASamples;

    if (_postAASamples != samples) {
        _postAASamples = samples;
        _fxaaConstants.set("dvd_qualityMultiplier", PushConstantType::INT, _postAASamples);
    }

    if (_idleCount == 0) {
        _useSMAA = _ID_RT(config.rendering.postAAType) == _ID("SMAA");
    }

    _idleCount = (++_idleCount % 60);
}

void PostAAPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
}

/// This is tricky as we use our screen as both input and output
void PostAAPreRenderOperator::execute(GFX::CommandBuffer& bufferInOut) {
    STUBBED("ToDo: Move PostAA to compute shaders to avoid a blit and RT swap. -Ionut");
    RenderTargetHandle ldrTarget = _parent.outputRT();

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgram = (_useSMAA ? _smaa : _fxaa);

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    GFX::BlitRenderTargetCommand blitRTCommand;
    blitRTCommand._source = ldrTarget._targetID;
    blitRTCommand._destination = _samplerCopy._targetID;
    GFX::BlitRenderTarget(bufferInOut, blitRTCommand);

    TextureData data0 = _samplerCopy._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.addTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    // Apply FXAA/SMAA to the specified render target
    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = ldrTarget._targetID;
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);

    GenericDrawCommand pointsCmd;
    pointsCmd.primitiveType(PrimitiveType::API_POINTS);
    pointsCmd.drawCount(1);

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _fxaaConstants;
    GFX::SendPushConstants(bufferInOut, pushConstantsCommand);

    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(pointsCmd);
    GFX::AddDrawCommands(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);
}

};