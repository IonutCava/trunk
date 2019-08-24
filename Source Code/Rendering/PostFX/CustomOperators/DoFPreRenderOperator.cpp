#include "stdafx.h"

#include "Headers/DoFPreRenderOperator.h"

#include "Core/Headers/Console.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

DoFPreRenderOperator::DoFPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_DEPTH_OF_FIELD)
{
    vector<RTAttachmentDescriptor> att = {
        { parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getDescriptor(), RTAttachmentType::Colour },
    };

    RenderTargetDescriptor desc = {};
    desc._name = "DoF";
    desc._resolution = vec2<U16>(parent.inputRT()._rt->getWidth(), parent.inputRT()._rt->getHeight());
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    _samplerCopy = _context.renderTargetPool().allocateRT(desc);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "FullScreenQuad";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "DepthOfField.glsl";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor dof("DepthOfField");
    dof.setPropertyDescriptor(shaderDescriptor);
    _dofShader = CreateResource<ShaderProgram>(cache, dof);
}

DoFPreRenderOperator::~DoFPreRenderOperator()
{
}

void DoFPreRenderOperator::idle(const Configuration& config) {
    ACKNOWLEDGE_UNUSED(config);
}

void DoFPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);
}

void DoFPreRenderOperator::execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    // Copy current screen
    GFX::BlitRenderTargetCommand blitRTCommand;
    blitRTCommand._source = _parent.inputRT()._targetID;
    blitRTCommand._destination = _samplerCopy._targetID;
    blitRTCommand._blitColours.emplace_back();
    GFX::EnqueueCommand(bufferInOut, blitRTCommand);

    TextureData data0 = _samplerCopy._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    TextureData depthData = _parent.inputRT()._rt->getAttachment(RTAttachmentType::Depth, 0).texture()->getData();

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _dofShader->getGUID();

    GenericDrawCommand pointsCmd;
    pointsCmd._primitiveType = PrimitiveType::API_POINTS;
    pointsCmd._drawCount = 1;

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
    descriptorSetCmd._set._textureData.setTexture(depthData, to_U8(ShaderProgram::TextureUsage::UNIT1));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = _parent.inputRT()._targetID;
    beginRenderPassCmd._descriptor = _screenOnlyDraw;
    beginRenderPassCmd._name = "DO_DOF_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::DrawCommand drawCmd = { pointsCmd };
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}
};