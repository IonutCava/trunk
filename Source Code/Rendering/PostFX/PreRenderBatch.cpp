#include "stdafx.h"

#include "Headers/PreRenderBatch.h"
#include "Headers/PreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DofPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/PostAAPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"

namespace Divide {

PreRenderBatch::PreRenderBatch(GFXDevice& context, ResourceCache& cache)
    : _context(context),
      _resCache(cache),
      _adaptiveExposureControl(true),
      _debugOperator(nullptr),
      _renderTarget(RenderTargetUsage::SCREEN),
      _toneMap(nullptr)
{
}

PreRenderBatch::~PreRenderBatch()
{
    destroy();
}

void PreRenderBatch::destroy() {
    for (OperatorBatch& batch : _operators) {
        MemoryManager::DELETE_VECTOR(batch);
        batch.clear();
    }
    
    _context.renderTargetPool().deallocateRT(_previousLuminance);
    _context.renderTargetPool().deallocateRT(_currentLuminance);
    _context.renderTargetPool().deallocateRT(_postFXOutput);
}

void PreRenderBatch::init(RenderTargetID renderTarget) {
    assert(_postFXOutput._targetID._usage == RenderTargetUsage::COUNT);
    _renderTarget = renderTarget;

    const RenderTarget& rt = *inputRT()._rt;

    SamplerDescriptor screenSampler;
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.setFilters(TextureFilter::LINEAR);
    screenSampler.setAnisotropy(0);

    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RGBA8,
                                       GFXDataFormat::UNSIGNED_BYTE);
    outputDescriptor.setSampler(screenSampler);
    {
        //Colour0 holds the LDR screen texture
        vectorImpl<RTAttachmentDescriptor> att = {
            { outputDescriptor, RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "PostFXOutput";
        desc._resolution = vec2<U16>(rt.getWidth(), rt.getHeight());
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _postFXOutput = _context.renderTargetPool().allocateRT(desc);
    }

    SamplerDescriptor lumaSampler;
    lumaSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    lumaSampler.setMinFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);

    TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D,
                                     GFXImageFormat::RED16F,
                                     GFXDataFormat::FLOAT_16);
    lumaDescriptor.setSampler(lumaSampler);
    lumaDescriptor.toggleAutomaticMipMapGeneration(false);
    {
        // make the texture square sized and power of two
        U16 lumaRez = to_U16(nextPOW2(to_U32(rt.getWidth() / 3.0f)));

        vectorImpl<RTAttachmentDescriptor> att = {
            { lumaDescriptor, RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "Luminance";
        desc._resolution = vec2<U16>(lumaRez);
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _currentLuminance = _context.renderTargetPool().allocateRT(desc);
    }
    lumaSampler.setFilters(TextureFilter::LINEAR);
    lumaDescriptor.setSampler(lumaSampler);
    {
        vectorImpl<RTAttachmentDescriptor> att = {
            { lumaDescriptor, RTAttachmentType::Colour, 0, DefaultColours::BLACK() },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "PreviousLuminance";
        desc._resolution = vec2<U16>(1);
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _previousLuminance = _context.renderTargetPool().allocateRT(desc);
    }
    _toneMapConstants.set("luminanceMipLevel",
                          PushConstantType::INT,
                          to_I32(_currentLuminance
                          ._rt
                          ->getAttachment(RTAttachmentType::Colour, 0)
                          .texture()
                          ->getMaxMipLevel()));
    _toneMapConstants.set("exposureMipLevel",
                          PushConstantType::INT,
                          0);
    // Order is very important!
    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    hdrBatch.push_back(MemoryManager_NEW SSAOPreRenderOperator(_context, *this, _resCache));
    hdrBatch.push_back(MemoryManager_NEW DoFPreRenderOperator(_context, *this, _resCache));
    hdrBatch.push_back(MemoryManager_NEW BloomPreRenderOperator(_context, *this, _resCache));

    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];
    ldrBatch.push_back(MemoryManager_NEW PostAAPreRenderOperator(_context, *this, _resCache));

    ResourceDescriptor toneMap("toneMap");
    toneMap.setThreadedLoading(false);
    _toneMap = CreateResource<ShaderProgram>(_resCache, toneMap);

    ResourceDescriptor toneMapAdaptive("toneMap.Adaptive");
    toneMapAdaptive.setThreadedLoading(false);
    toneMapAdaptive.setPropertyList("USE_ADAPTIVE_LUMINANCE");
    _toneMapAdaptive = CreateResource<ShaderProgram>(_resCache, toneMapAdaptive);

    ResourceDescriptor luminanceCalc("luminanceCalc");
    luminanceCalc.setThreadedLoading(false);
    _luminanceCalc = CreateResource<ShaderProgram>(_resCache, luminanceCalc);

    //_debugOperator = hdrBatch[0];
}

TextureData PreRenderBatch::getOutput() {
    TextureData ret;
    if (_debugOperator != nullptr) {
        ret = _debugOperator->getDebugOutput();
    } else {
        ret = _postFXOutput._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    }
    return ret;
}

void PreRenderBatch::idle(const Configuration& config) {
    for (OperatorBatch& batch : _operators) {
        for (PreRenderOperator* op : batch) {
            if (op != nullptr) {
                op->idle(config);
            }
        }
    }
}

RenderTargetHandle PreRenderBatch::inputRT() const {
    return RenderTargetHandle(_renderTarget, &_context.renderTargetPool().renderTarget(_renderTarget));
}

RenderTargetHandle& PreRenderBatch::outputRT() {
    return _postFXOutput;
}

void PreRenderBatch::execute(const FilterStack& stack, GFX::CommandBuffer& buffer) {
    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._shaderProgram = _luminanceCalc;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);

    TextureData data0 = inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();

    if (_adaptiveExposureControl) {
        // Compute Luminance
        // Step 1: Luminance calc
        TextureData data1 = _previousLuminance._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();

        GFX::BeginRenderPassCommand beginRenderPassCmd;
        beginRenderPassCmd._target = _currentLuminance._targetID;
        GFX::BeginRenderPass(buffer, beginRenderPassCmd);

        GFX::BindPipelineCommand pipelineCmd;
        pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
        GFX::BindPipeline(buffer, pipelineCmd);

        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set._textureData.addTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
        descriptorSetCmd._set._textureData.addTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
        GFX::BindDescriptorSets(buffer, descriptorSetCmd);

        GFX::DrawCommand drawCmd;
        drawCmd._drawCommands.push_back(triangleCmd);
        GFX::AddDrawCommands(buffer, drawCmd);

        GFX::EndRenderPassCommand endRenderPassCmd;
        GFX::EndRenderPass(buffer, endRenderPassCmd);

        // Use previous luminance to control adaptive exposure
        GFX::BlitRenderTargetCommand blitRTCommand;
        blitRTCommand._source = _currentLuminance._targetID;
        blitRTCommand._destination = _previousLuminance._targetID;
        GFX::BlitRenderTarget(buffer, blitRTCommand);
    }

    // Execute all HDR based operators
    for (PreRenderOperator* op : hdrBatch) {
        if (op != nullptr && stack[to_U32(op->operatorType())] > 0) {
            op->execute(buffer);
        }
    }

    pipelineDescriptor._shaderProgram = (_adaptiveExposureControl ? _toneMapAdaptive : _toneMap);
    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(buffer, pipelineCmd);

    // ToneMap and generate LDR render target (Alpha channel contains pre-toneMapped luminance value)
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.addTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));

    if (_adaptiveExposureControl) {
        TextureData data1 = _currentLuminance._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
        descriptorSetCmd._set._textureData.addTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
    }
    GFX::BindDescriptorSets(buffer, descriptorSetCmd);

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = _postFXOutput._targetID;
    GFX::BeginRenderPass(buffer, beginRenderPassCmd);

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _toneMapConstants;
    GFX::SendPushConstants(buffer, pushConstantsCommand);

    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(triangleCmd);
    GFX::AddDrawCommands(buffer, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EndRenderPass(buffer, endRenderPassCmd);

    // Execute all LDR based operators
    for (PreRenderOperator* op : ldrBatch) {
        if (op != nullptr && stack[to_U32(op->operatorType())] > 0) {
            op->execute(buffer);
        }
    }
}

void PreRenderBatch::reshape(U16 width, U16 height) {
    // make the texture square sized and power of two
    U16 lumaRez = to_U16(nextPOW2(to_U32(width / 3.0f)));

    _currentLuminance._rt->resize(lumaRez, lumaRez);
    _previousLuminance._rt->resize(1, 1);

    _toneMapConstants.set("luminanceMipLevel",
                          PushConstantType::UINT, 
                          _currentLuminance
                          ._rt
                          ->getAttachment(RTAttachmentType::Colour, 0)
                          .texture()
                          ->getMaxMipLevel());

    for (OperatorBatch& batch : _operators) {
        for (PreRenderOperator* op : batch) {
            if (op != nullptr) {
                op->reshape(width, height);
            }
        }
    }

    _postFXOutput._rt->resize(width, height);
}
};