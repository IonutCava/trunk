#include "stdafx.h"

#include "Headers/BloomPreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

BloomPreRenderOperator::BloomPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache)
    : PreRenderOperator(context, parent, cache, FilterType::FILTER_BLOOM),
      _bloomFactor(0.8f),
      _bloomThreshold(0.75f)
{
    vec2<U16> res(parent.inputRT()._rt->getWidth(), parent.inputRT()._rt->getHeight());

    vectorSTD<RTAttachmentDescriptor> att = {
        {
            parent.inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->descriptor(),
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
    desc._resolution = vec2<U16>(res / 4.0f);
    _bloomOutput = _context.renderTargetPool().allocateRT(desc);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "FullScreenQuad";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "bloom.glsl";
    fragModule._variant = "BloomCalc";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor bloomCalc("BloomCalc");
    bloomCalc.propertyDescriptor(shaderDescriptor);
    _bloomCalc = CreateResource<ShaderProgram>(cache, bloomCalc);

    fragModule._variant = "BloomApply";
    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor bloomApply("BloomApply");
    bloomApply.propertyDescriptor(shaderDescriptor);
    _bloomApply = CreateResource<ShaderProgram>(cache, bloomApply);

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _bloomCalc->getGUID();

    _bloomCalcPipeline = _context.newPipeline(pipelineDescriptor);

    pipelineDescriptor._shaderProgramHandle = _bloomApply->getGUID();
    _bloomApplyPipeline = _context.newPipeline(pipelineDescriptor);

    factor(_context.context().config().rendering.postFX.bloomFactor);
}

BloomPreRenderOperator::~BloomPreRenderOperator() {
    _context.renderTargetPool().deallocateRT(_bloomOutput);
    _context.renderTargetPool().deallocateRT(_bloomBlurBuffer[0]);
    _context.renderTargetPool().deallocateRT(_bloomBlurBuffer[1]);
}

void BloomPreRenderOperator::reshape(U16 width, U16 height) {
    PreRenderOperator::reshape(width, height);

    U16 w = to_U16(width / 4.0f);
    U16 h = to_U16(height / 4.0f);
    _bloomOutput._rt->resize(w, h);
    _bloomBlurBuffer[0]._rt->resize(width, height);
    _bloomBlurBuffer[1]._rt->resize(width, height);
}

void BloomPreRenderOperator::factor(F32 val) {
    _bloomFactor = val;
    _bloomApplyConstants.set(_ID("bloomFactor"), GFX::PushConstantType::FLOAT, _bloomFactor);
    _context.context().config().rendering.postFX.bloomFactor = val;
}

void BloomPreRenderOperator::luminanceThreshold(F32 val) {
    _bloomThreshold = val;
    _bloomCalcConstants.set(_ID("luminanceThreshold"), GFX::PushConstantType::FLOAT, _bloomThreshold);
}

void BloomPreRenderOperator::prepare(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(camera);
    ACKNOWLEDGE_UNUSED(bufferInOut);
}

// Order: luminance calc -> bloom -> tonemap
void BloomPreRenderOperator::execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) {

    GenericDrawCommand triangleCmd = {};
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;

    RenderTargetHandle screen = _parent.inputRT();
    TextureData data = screen._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data(); //screen
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.setTexture(data, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _bloomCalcPipeline });
    GFX::EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _bloomCalcConstants });

     // Step 1: generate bloom

    // render all of the "bright spots"
    GFX::BeginRenderPassCommand beginRenderPassCmd = {};
    beginRenderPassCmd._target = _bloomOutput._targetID;
    beginRenderPassCmd._name = "DO_BLOOM_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::DrawCommand drawCmd = { triangleCmd };
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

    // Step 2: blur bloom
    _context.blurTarget(_bloomOutput,
                        _bloomBlurBuffer[0],
                        _bloomBlurBuffer[1],
                        RTAttachmentType::Colour,
                        0,
                        10,
                        bufferInOut);

    // Step 3: apply bloom
    GFX::BlitRenderTargetCommand blitRTCommand = {};
    blitRTCommand._source = screen._targetID;
    blitRTCommand._destination = _bloomBlurBuffer[0]._targetID;
    blitRTCommand._blitColours.emplace_back();
    GFX::EnqueueCommand(bufferInOut, blitRTCommand);

    TextureData data0 = _bloomBlurBuffer[0]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data(); //Screen
    TextureData data1 = _bloomBlurBuffer[1]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data(); //Bloom

    descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
    descriptorSetCmd._set._textureData.setTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _bloomApplyPipeline });
    GFX::EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _bloomApplyConstants });

    beginRenderPassCmd._target = screen._targetID;
    beginRenderPassCmd._descriptor = _screenOnlyDraw;
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
}

TextureData BloomPreRenderOperator::getDebugOutput() const {
    return _bloomBlurBuffer[1]._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data(); //Bloom
}

};