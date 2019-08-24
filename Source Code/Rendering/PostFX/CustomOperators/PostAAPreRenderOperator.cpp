#include "stdafx.h"

#include "Headers/PostAAPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Core/Headers/Configuration.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

PostAAPreRenderOperator::PostAAPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_SS_ANTIALIASING),
      _useSMAA(false),
      _postAASamples(0),
      _idleCount(0)
{
    vector<RTAttachmentDescriptor> att = {
        { parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getDescriptor(), RTAttachmentType::Colour },
    };

    RenderTargetDescriptor desc = {};
    desc._name = "PostAA";
    desc._resolution = vec2<U16>(parent.inputRT()._rt->getWidth(), parent.inputRT()._rt->getHeight());
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    _samplerCopy = _context.renderTargetPool().allocateRT(desc);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "Dummy";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "FXAA.glsl";

    ShaderProgramDescriptor aaShaderDescriptor = {};
    aaShaderDescriptor._modules.push_back(vertModule);
    aaShaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor fxaa("FXAA");
    fxaa.setPropertyDescriptor(aaShaderDescriptor);
    fxaa.waitForReady(false);
    _fxaa = CreateResource<ShaderProgram>(cache, fxaa);

    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "Dummy";

    fragModule._sourceFile = "SMAA.glsl";

    ShaderModuleDescriptor geomModule = {};
    geomModule._moduleType = ShaderType::GEOMETRY;
    geomModule._sourceFile = "SMAA.glsl";

    aaShaderDescriptor = {};
    aaShaderDescriptor._modules.push_back(vertModule);
    aaShaderDescriptor._modules.push_back(geomModule);
    aaShaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor smaa("SMAA");
    smaa.setPropertyDescriptor(aaShaderDescriptor);
    smaa.waitForReady(false);
    _smaa = CreateResource<ShaderProgram>(cache, smaa);
}

PostAAPreRenderOperator::~PostAAPreRenderOperator()
{
}

void PostAAPreRenderOperator::idle(const Configuration& config) {
    I32 samples = config.rendering.postFX.postAASamples;

    if (_postAASamples != samples) {
        _postAASamples = samples;
        _fxaaConstants.set("dvd_qualityMultiplier", GFX::PushConstantType::INT, _postAASamples);
    }

    if (_idleCount == 0) {
        _useSMAA = _ID(config.rendering.postFX.postAAType.c_str()) == _ID("SMAA");
    }

    _idleCount = (++_idleCount % 60);
}

void PostAAPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
}

/// This is tricky as we use our screen as both input and output
void PostAAPreRenderOperator::execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    STUBBED("ToDo: Move PostAA to compute shaders to avoid a blit and RT swap. -Ionut");
    RenderTargetHandle ldrTarget = _parent.outputRT();

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = (_useSMAA ? _smaa : _fxaa)->getGUID();

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    GFX::BlitRenderTargetCommand blitRTCommand;
    blitRTCommand._source = ldrTarget._targetID;
    blitRTCommand._destination = _samplerCopy._targetID;
    blitRTCommand._blitColours.emplace_back();
    GFX::EnqueueCommand(bufferInOut, blitRTCommand);

    TextureData data0 = _samplerCopy._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    // Apply FXAA/SMAA to the specified render target
    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = ldrTarget._targetID;
    beginRenderPassCmd._name = "DO_POSTAA_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GenericDrawCommand pointsCmd;
    pointsCmd._primitiveType = PrimitiveType::API_POINTS;
    pointsCmd._drawCount = 1;

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _fxaaConstants;
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

    GFX::DrawCommand drawCmd = { pointsCmd };
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

};