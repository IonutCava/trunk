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

    SamplerDescriptor screenSampler = {};
    screenSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    screenSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    screenSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    screenSampler._minFilter = TextureFilter::LINEAR;
    screenSampler._magFilter = TextureFilter::LINEAR;
    screenSampler._anisotropyLevel = 0;

    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGB, GFXDataFormat::UNSIGNED_BYTE);
    outputDescriptor.setSampler(screenSampler);
    
    {
        //Colour0 holds the LDR screen texture
        vector<RTAttachmentDescriptor> att = {
            { outputDescriptor, RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "PostFXOutput";
        desc._resolution = vec2<U16>(rt.getWidth(), rt.getHeight());
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _postFXOutput = _context.renderTargetPool().allocateRT(desc);
    }

    SamplerDescriptor lumaSampler = {};
    lumaSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    lumaSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    lumaSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    lumaSampler._minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
    lumaSampler._magFilter = TextureFilter::LINEAR;

    TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RED, GFXDataFormat::FLOAT_16);
    lumaDescriptor.setSampler(lumaSampler);
    lumaDescriptor.automaticMipMapGeneration(true);
    {
        // make the texture square sized and power of two
        U16 lumaRez = to_U16(nextPOW2(to_U32(rt.getWidth() / 3.0f)));

        vector<RTAttachmentDescriptor> att = {
            { lumaDescriptor, RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "Luminance";
        desc._resolution = vec2<U16>(lumaRez);
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _currentLuminance = _context.renderTargetPool().allocateRT(desc);
    }

    lumaSampler._minFilter = TextureFilter::LINEAR;
    lumaSampler._anisotropyLevel = 0;

    lumaDescriptor.setSampler(lumaSampler);
    {
        vector<RTAttachmentDescriptor> att = {
            {
                lumaDescriptor,
                RTAttachmentType::Colour,
                0,
                DefaultColours::BLACK
            },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "PreviousLuminance";
        desc._resolution = vec2<U16>(1);
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _previousLuminance = _context.renderTargetPool().allocateRT(desc);
    }
    _toneMapConstants.set("luminanceMipLevel",
                          GFX::PushConstantType::INT,
                          to_I32(_currentLuminance
                          ._rt
                          ->getAttachment(RTAttachmentType::Colour, 0)
                          .texture()
                          ->getMaxMipLevel()));
    _toneMapConstants.set("exposureMipLevel",
                          GFX::PushConstantType::INT,
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

    ShaderProgramDescriptor toneMapAdaptiveDescriptor;
    toneMapAdaptiveDescriptor._defines.push_back(std::make_pair("USE_ADAPTIVE_LUMINANCE", true));
    toneMapAdaptive.setPropertyDescriptor(toneMapAdaptiveDescriptor);

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

void PreRenderBatch::execute(const Camera& camera, U32 filterStack, GFX::CommandBuffer& buffer) {
    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];

    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;


    TextureData data0 = inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();

    if (_adaptiveExposureControl) {
        // Compute Luminance
        // Step 1: Luminance calc
        TextureData data1 = _previousLuminance._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();

        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        beginRenderPassCmd._target = _currentLuminance._targetID;
        beginRenderPassCmd._name = "DO_LUMINANCE_PASS";
        GFX::EnqueueCommand(buffer, beginRenderPassCmd);

        pipelineDescriptor._shaderProgramHandle = _luminanceCalc->getID();

        GFX::BindPipelineCommand pipelineCmd = {};
        pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
        GFX::EnqueueCommand(buffer, pipelineCmd);

        // We don't know if our screen target has been resolved
        GFX::ResolveRenderTargetCommand resolveCmd = { };
        resolveCmd._source = inputRT()._targetID;
        resolveCmd._resolveColours = true;
        resolveCmd._resolveDepth = false;

        GFX::EnqueueCommand(buffer, resolveCmd);
        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
        descriptorSetCmd._set._textureData.setTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
        GFX::EnqueueCommand(buffer, descriptorSetCmd);

        GFX::DrawCommand drawCmd = {};
        drawCmd._drawCommands.push_back(triangleCmd);
        GFX::EnqueueCommand(buffer, drawCmd);

        GFX::EndRenderPassCommand endRenderPassCmd = {};
        GFX::EnqueueCommand(buffer, endRenderPassCmd);

        // Use previous luminance to control adaptive exposure
        GFX::BlitRenderTargetCommand blitRTCommand = {};
        blitRTCommand._source = _currentLuminance._targetID;
        blitRTCommand._destination = _previousLuminance._targetID;
        blitRTCommand._blitColours.emplace_back();
        GFX::EnqueueCommand(buffer, blitRTCommand);
    }

    // Execute all HDR based operators
    for (PreRenderOperator* op : hdrBatch) {
        if (op != nullptr && BitCompare(filterStack, op->operatorType())) {
            op->execute(camera, buffer);
        }
    }

    // Post-HDR batch, our screen target has been written to again
    GFX::ResolveRenderTargetCommand resolveCmd = { };
    resolveCmd._source = inputRT()._targetID;
    resolveCmd._resolveColours = true;
    resolveCmd._resolveDepth = false;
    GFX::EnqueueCommand(buffer, resolveCmd);

    pipelineDescriptor._shaderProgramHandle = (_adaptiveExposureControl ? _toneMapAdaptive : _toneMap)->getID();
    GFX::BindPipelineCommand pipelineCmd = {};
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(buffer, pipelineCmd);

    // ToneMap and generate LDR render target (Alpha channel contains pre-toneMapped luminance value)
    GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
    descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));

    if (_adaptiveExposureControl) {
        TextureData data1 = _currentLuminance._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
        descriptorSetCmd._set._textureData.setTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
    }
    GFX::EnqueueCommand(buffer, descriptorSetCmd);

    GFX::BeginRenderPassCommand beginRenderPassCmd = {};
    beginRenderPassCmd._target = _postFXOutput._targetID;
    beginRenderPassCmd._name = "DO_TONEMAP_PASS";
    GFX::EnqueueCommand(buffer, beginRenderPassCmd);

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants = _toneMapConstants;
    GFX::EnqueueCommand(buffer, pushConstantsCommand);

    GFX::DrawCommand drawCmd = {};
    drawCmd._drawCommands.push_back(triangleCmd);
    GFX::EnqueueCommand(buffer, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd = {};
    GFX::EnqueueCommand(buffer, endRenderPassCmd);

    // Execute all LDR based operators
    for (PreRenderOperator* op : ldrBatch) {
        if (op != nullptr && BitCompare(filterStack, op->operatorType())) {
            op->execute(camera, buffer);
        }
    }
}

void PreRenderBatch::reshape(U16 width, U16 height) {
    // make the texture square sized and power of two
    U16 lumaRez = to_U16(nextPOW2(to_U32(width / 3.0f)));

    _currentLuminance._rt->resize(lumaRez, lumaRez);
    _previousLuminance._rt->resize(1, 1);

    _toneMapConstants.set("luminanceMipLevel",
                          GFX::PushConstantType::UINT,
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