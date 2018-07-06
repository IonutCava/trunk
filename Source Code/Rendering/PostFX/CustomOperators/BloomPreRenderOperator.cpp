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
    bloomCalc.setThreadedLoading(false);
    _bloomCalc = CreateResource<ShaderProgram>(cache, bloomCalc);

    ResourceDescriptor bloomApply("bloom.BloomApply");
    bloomApply.setThreadedLoading(false);
    _bloomApply = CreateResource<ShaderProgram>(cache, bloomApply);

    ResourceDescriptor blur("blur");
    blur.setThreadedLoading(false);
    _blur = CreateResource<ShaderProgram>(cache, blur);

    _bloomCalcConstants.set("kernelSize", GFX::PushConstantType::INT, 10);
    _horizBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurHorizontal");
    _vertBlur = _blur->GetSubroutineIndex(ShaderType::FRAGMENT, "blurVertical");
}

BloomPreRenderOperator::~BloomPreRenderOperator() {
    _context.renderTargetPool().deallocateRT(_bloomOutput);
    _context.renderTargetPool().deallocateRT(_bloomBlurBuffer[0]);
    _context.renderTargetPool().deallocateRT(_bloomBlurBuffer[1]);
}

void BloomPreRenderOperator::idle(const Configuration& config) {
    _bloomFactor = config.rendering.bloomFactor;
}

void BloomPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    U16 w = to_U16(width / 4.0f);
    U16 h = to_U16(height / 4.0f);
    _bloomOutput._rt->resize(w, h);
    _bloomBlurBuffer[0]._rt->resize(width, height);
    _bloomBlurBuffer[1]._rt->resize(width, height);

    _bloomCalcConstants.set("size", GFX::PushConstantType::VEC2, vec2<F32>(width, height));
}

// Order: luminance calc -> bloom -> tonemap
void BloomPreRenderOperator::execute(GFX::CommandBuffer& bufferInOut) {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);

    RenderTargetHandle screen = _parent.inputRT();

    pipelineDescriptor._shaderProgramHandle = _bloomCalc->getID();
    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = &_context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

     // Step 1: generate bloom
    TextureData data = screen._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData(); //screen

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    // render all of the "bright spots"
    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = _bloomOutput._targetID;
    beginRenderPassCmd._name = "DO_BLOOM_PASS";
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);

    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(triangleCmd);
    GFX::AddDrawCommands(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);

    // Step 2: blur bloom
    // Blur horizontally
    pipelineDescriptor._shaderProgramHandle = _blur->getID();
    pipelineDescriptor._shaderFunctions[to_base(ShaderType::FRAGMENT)].push_back(_horizBlur);
    pipelineCmd._pipeline = &_context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    data = _bloomOutput._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    descriptorSetCmd._set._textureData.clear();
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    beginRenderPassCmd._target = _bloomBlurBuffer[0]._targetID;
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _bloomCalcConstants;
    GFX::SendPushConstants(bufferInOut, pushConstantsCommand);

    GFX::AddDrawCommands(bufferInOut, drawCmd);

    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);

    // Blur vertically (recycle the render target. We have a copy)
    pipelineDescriptor._shaderProgramHandle = _blur->getID();
    pipelineDescriptor._shaderFunctions[to_base(ShaderType::FRAGMENT)].front() = _vertBlur;
    pipelineCmd._pipeline = &_context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    data = _bloomBlurBuffer[0]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    descriptorSetCmd._set._textureData.clear();
    descriptorSetCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    beginRenderPassCmd._target = _bloomBlurBuffer[1]._targetID;
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);
    GFX::SendPushConstants(bufferInOut, pushConstantsCommand);

    GFX::AddDrawCommands(bufferInOut, drawCmd);

    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);
        
    // Step 3: apply bloom
    GFX::BlitRenderTargetCommand blitRTCommand;
    blitRTCommand._source = screen._targetID;
    blitRTCommand._destination = _bloomBlurBuffer[0]._targetID;
    GFX::BlitRenderTarget(bufferInOut, blitRTCommand);

    TextureData data0 = _bloomBlurBuffer[0]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData(); //Screen
    TextureData data1 = _bloomBlurBuffer[1]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData(); //Bloom

    descriptorSetCmd._set._textureData.clear();
    descriptorSetCmd._set._textureData.addTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
    descriptorSetCmd._set._textureData.addTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
    GFX::BindDescriptorSets(bufferInOut, descriptorSetCmd);

    pipelineDescriptor._shaderProgramHandle = _bloomApply->getID();
    pipelineDescriptor._shaderFunctions[to_base(ShaderType::FRAGMENT)].clear();
    pipelineCmd._pipeline = &_context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(bufferInOut, pipelineCmd);

    _bloomApplyConstants.set("bloomFactor", GFX::PushConstantType::FLOAT, _bloomFactor);
    pushConstantsCommand._constants = _bloomApplyConstants;
    GFX::SendPushConstants(bufferInOut, pushConstantsCommand);

    beginRenderPassCmd._target = screen._targetID;
    beginRenderPassCmd._descriptor = _screenOnlyDraw;
    GFX::BeginRenderPass(bufferInOut, beginRenderPassCmd);

    GFX::AddDrawCommands(bufferInOut, drawCmd);

    GFX::EndRenderPass(bufferInOut, endRenderPassCmd);
}

TextureData BloomPreRenderOperator::getDebugOutput() const {
    return TextureData();
}

};