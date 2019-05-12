#include "stdafx.h"

#include "Headers/BloomPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

BloomPreRenderOperator::BloomPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_BLOOM),
      _bloomFactor(0.0f)
{
    vec2<U16> res(parent.inputRT()._rt->getWidth(), parent.inputRT()._rt->getHeight());

    vector<RTAttachmentDescriptor> att = {
        {
            parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getDescriptor(),
            RTAttachmentType::Colour,
            0,
            DefaultColours::BLACK
        },
    };

    RenderTargetDescriptor desc = {};
    desc._resolution = res;
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    desc._name = "Bloom_Blur_0";
    _bloomBlurBuffer[0] = _context.renderTargetPool().allocateRT(desc);
    desc._name = "Bloom_Blur_1";
    _bloomBlurBuffer[1] = _context.renderTargetPool().allocateRT(desc);

    desc._name = "Bloom";
    desc._resolution = vec2<U16>(to_U16(res.w / 4.0f), to_U16(res.h / 4.0f));
    _bloomOutput = _context.renderTargetPool().allocateRT(desc);

    ResourceDescriptor bloomCalc("bloom.BloomCalc");
    _bloomCalc = CreateResource<ShaderProgram>(cache, bloomCalc);

    ResourceDescriptor bloomApply("bloom.BloomApply");
    _bloomApply = CreateResource<ShaderProgram>(cache, bloomApply);
}

BloomPreRenderOperator::~BloomPreRenderOperator() {
    _context.renderTargetPool().deallocateRT(_bloomOutput);
    _context.renderTargetPool().deallocateRT(_bloomBlurBuffer[0]);
    _context.renderTargetPool().deallocateRT(_bloomBlurBuffer[1]);
}

void BloomPreRenderOperator::idle(const Configuration& config) {
    _bloomFactor = config.rendering.postFX.bloomFactor;
}

void BloomPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    U16 w = to_U16(width / 4.0f);
    U16 h = to_U16(height / 4.0f);
    _bloomOutput._rt->resize(w, h);
    _bloomBlurBuffer[0]._rt->resize(width, height);
    _bloomBlurBuffer[1]._rt->resize(width, height);
}

// Order: luminance calc -> bloom -> tonemap
void BloomPreRenderOperator::execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;

    RenderTargetHandle screen = _parent.inputRT();
    TextureData data = screen._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData(); //screen
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.setTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    pipelineDescriptor._shaderProgramHandle = _bloomCalc->getGUID();
    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

     // Step 1: generate bloom

    // render all of the "bright spots"
    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = _bloomOutput._targetID;
    beginRenderPassCmd._name = "DO_BLOOM_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(triangleCmd);
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

    // Step 2: blur bloom
    _context.blurTarget(_bloomOutput,
                        _bloomBlurBuffer[0],
                        _bloomBlurBuffer[1],
                        RTAttachmentType::Colour,
                        0,
                        10,
                        bufferInOut);

    // Step 3: apply bloom
    GFX::BlitRenderTargetCommand blitRTCommand;
    blitRTCommand._source = screen._targetID;
    blitRTCommand._destination = _bloomBlurBuffer[0]._targetID;
    blitRTCommand._blitColours.emplace_back();
    GFX::EnqueueCommand(bufferInOut, blitRTCommand);

    TextureData data0 = _bloomBlurBuffer[0]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData(); //Screen
    TextureData data1 = _bloomBlurBuffer[1]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData(); //Bloom

    descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
    descriptorSetCmd._set._textureData.setTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    pipelineDescriptor._shaderProgramHandle = _bloomApply->getGUID();
    pipelineDescriptor._shaderFunctions[to_base(ShaderType::FRAGMENT)].clear();
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    _bloomApplyConstants.set("bloomFactor", GFX::PushConstantType::FLOAT, _bloomFactor);
    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _bloomApplyConstants;
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

    beginRenderPassCmd._target = screen._targetID;
    beginRenderPassCmd._descriptor = _screenOnlyDraw;
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EnqueueCommand(bufferInOut, drawCmd);

    endRenderPassCmd._autoResolveMSAAColour = true;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

TextureData BloomPreRenderOperator::getDebugOutput() const {
    return _bloomBlurBuffer[1]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData(); //Bloom
}

};