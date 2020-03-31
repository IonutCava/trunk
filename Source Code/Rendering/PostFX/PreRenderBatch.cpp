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
        MemoryManager::DELETE_CONTAINER(batch);
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
    screenSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.minFilter(TextureFilter::LINEAR);
    screenSampler.magFilter(TextureFilter::LINEAR);
    screenSampler.anisotropyLevel(0);

    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGB, GFXDataFormat::UNSIGNED_BYTE);
    outputDescriptor.samplerDescriptor(screenSampler);
    
    {
        //Colour0 holds the LDR screen texture
        std::vector<RTAttachmentDescriptor> att = {
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
    lumaSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    lumaSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    lumaSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    lumaSampler.minFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    lumaSampler.magFilter(TextureFilter::LINEAR);

    TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RED, GFXDataFormat::FLOAT_16);
    lumaDescriptor.samplerDescriptor(lumaSampler);
    lumaDescriptor.autoMipMaps(true);
    {
        // make the texture square sized and power of two
        U16 lumaRez = to_U16(nextPOW2(to_U32(rt.getWidth() / 3.0f)));

        std::vector<RTAttachmentDescriptor> att = {
            { lumaDescriptor, RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "Luminance";
        desc._resolution = vec2<U16>(lumaRez);
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _currentLuminance = _context.renderTargetPool().allocateRT(desc);
    }

    lumaSampler.minFilter(TextureFilter::LINEAR);
    lumaSampler.anisotropyLevel(0);

    lumaDescriptor.samplerDescriptor(lumaSampler);
    {
        std::vector<RTAttachmentDescriptor> att = {
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
    _toneMapConstants.set(_ID("luminanceMipLevel"),
                          GFX::PushConstantType::INT,
                          to_I32(_currentLuminance
                          ._rt
                          ->getAttachment(RTAttachmentType::Colour, 0)
                          .texture()
                          ->getMaxMipLevel()));
    _toneMapConstants.set(_ID("exposureMipLevel"),
                          GFX::PushConstantType::INT,
                          0);
    // Order is very important!
    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    hdrBatch.push_back(MemoryManager_NEW SSAOPreRenderOperator(_context, *this, _resCache));
    hdrBatch.push_back(MemoryManager_NEW DoFPreRenderOperator(_context, *this, _resCache));
    hdrBatch.push_back(MemoryManager_NEW BloomPreRenderOperator(_context, *this, _resCache));

    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];
    ldrBatch.push_back(MemoryManager_NEW PostAAPreRenderOperator(_context, *this, _resCache));
    {
        ShaderModuleDescriptor vertModule = {};
        vertModule._moduleType = ShaderType::VERTEX;
        vertModule._sourceFile = "baseVertexShaders.glsl";
        vertModule._variant = "FullScreenQuad";

        ShaderModuleDescriptor fragModule = {};
        fragModule._moduleType = ShaderType::FRAGMENT;
        fragModule._sourceFile = "toneMap.glsl";

        ShaderProgramDescriptor mapDescriptor1 = {};
        mapDescriptor1._modules.push_back(vertModule);
        mapDescriptor1._modules.push_back(fragModule);

        ResourceDescriptor toneMap("toneMap");
        toneMap.threaded(false);
        toneMap.propertyDescriptor(mapDescriptor1);

        _toneMap = CreateResource<ShaderProgram>(_resCache, toneMap);
    
        fragModule._defines.emplace_back("USE_ADAPTIVE_LUMINANCE", true);

        ShaderProgramDescriptor mapDescriptor2 = {};
        mapDescriptor2._modules.push_back(vertModule);
        mapDescriptor2._modules.push_back(fragModule);

        ShaderProgramDescriptor toneMapAdaptiveDescriptor;
        toneMapAdaptiveDescriptor._modules.push_back(vertModule);
        toneMapAdaptiveDescriptor._modules.push_back(fragModule);

        ResourceDescriptor toneMapAdaptive("toneMap.Adaptive");
        toneMapAdaptive.threaded(false);
        toneMapAdaptive.propertyDescriptor(toneMapAdaptiveDescriptor);

        _toneMapAdaptive = CreateResource<ShaderProgram>(_resCache, toneMapAdaptive);
    }
    {
        ShaderModuleDescriptor vertModule = {};
        vertModule._moduleType = ShaderType::VERTEX;
        vertModule._sourceFile = "baseVertexShaders.glsl";
        vertModule._variant = "FullScreenQuad";

        ShaderModuleDescriptor fragModule = {};
        fragModule._moduleType = ShaderType::FRAGMENT;
        fragModule._sourceFile = "luminanceCalc.glsl";

        ShaderProgramDescriptor calcDescriptor = {};
        calcDescriptor._modules.push_back(vertModule);
        calcDescriptor._modules.push_back(fragModule);

        ResourceDescriptor luminanceCalc("luminanceCalc");
        luminanceCalc.threaded(false);
        luminanceCalc.propertyDescriptor(calcDescriptor);
        _luminanceCalc = CreateResource<ShaderProgram>(_resCache, luminanceCalc);
    }

    //_debugOperator = hdrBatch[0];
}

TextureData PreRenderBatch::getOutput() {
    if (_debugOperator != nullptr) {
        return _debugOperator->getDebugOutput();
    }

    return _postFXOutput._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data();
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

void PreRenderBatch::onFilterEnabled(FilterType filter) {
    onFilterToggle(filter, true);
}

void PreRenderBatch::onFilterDisabled(FilterType filter) {
    onFilterToggle(filter, false);
}

void PreRenderBatch::onFilterToggle(FilterType filter, const bool state) {
    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];

    for (PreRenderOperator* op : hdrBatch) {
        if (op != nullptr && filter == op->operatorType()) {
            op->onToggle(state);
        }
    }
    for (PreRenderOperator* op : ldrBatch) {
        if (op != nullptr && filter == op->operatorType()) {
            op->onToggle(state);
        }
    }
}

void PreRenderBatch::prepare(const Camera& camera, U16 filterStack, GFX::CommandBuffer& bufferInOut) {
    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];

    for (PreRenderOperator* op : hdrBatch) {
        if (op != nullptr) {
            op->prepare(camera, bufferInOut);
        }
    }
    for (PreRenderOperator* op : ldrBatch) {
        if (op != nullptr) {
            op->prepare(camera, bufferInOut);
        }
    }
}

void PreRenderBatch::execute(const Camera& camera, U16 filterStack, GFX::CommandBuffer& bufferInOut) {
    static Pipeline* pipelineLumCalc = nullptr, * pipelineToneMap = nullptr, * pipelineToneMapAdaptive = nullptr;
    if (pipelineLumCalc == nullptr) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = _context.get2DStateBlock();

        pipelineDescriptor._shaderProgramHandle = _luminanceCalc->getGUID();
        pipelineLumCalc = _context.newPipeline(pipelineDescriptor);

        pipelineDescriptor._shaderProgramHandle = _toneMapAdaptive->getGUID();
        pipelineToneMapAdaptive = _context.newPipeline(pipelineDescriptor);

        pipelineDescriptor._shaderProgramHandle = _toneMap->getGUID();
        pipelineToneMap = _context.newPipeline(pipelineDescriptor);
    }

    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];

    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;


    TextureData data0 = inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data();

    if (_adaptiveExposureControl) {
        // Compute Luminance
        // Step 1: Luminance calc
        TextureData data1 = _previousLuminance._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data();

        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        beginRenderPassCmd._target = _currentLuminance._targetID;
        beginRenderPassCmd._name = "DO_LUMINANCE_PASS";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ pipelineLumCalc });

        // We don't know if our screen target has been resolved
        GFX::ResolveRenderTargetCommand resolveCmd = { };
        resolveCmd._source = inputRT()._targetID;
        resolveCmd._resolveColours = true;
        resolveCmd._resolveDepth = false;

        GFX::EnqueueCommand(bufferInOut, resolveCmd);

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));
        descriptorSetCmd._set._textureData.setTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::DrawCommand{ triangleCmd });
        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

        // Use previous luminance to control adaptive exposure
        GFX::BlitRenderTargetCommand blitRTCommand = {};
        blitRTCommand._source = _currentLuminance._targetID;
        blitRTCommand._destination = _previousLuminance._targetID;
        blitRTCommand._blitColours.emplace_back();
        GFX::EnqueueCommand(bufferInOut, blitRTCommand);
    }

    // Execute all HDR based operators
    for (PreRenderOperator* op : hdrBatch) {
        if (op != nullptr && BitCompare(filterStack, to_U16(op->operatorType()))) {
            op->execute(camera, bufferInOut);
        }
    }

    // Post-HDR batch, our screen target has been written to again
    GFX::ResolveRenderTargetCommand resolveCmd = { };
    resolveCmd._source = inputRT()._targetID;
    resolveCmd._resolveColours = true;
    resolveCmd._resolveDepth = false;
    GFX::EnqueueCommand(bufferInOut, resolveCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _adaptiveExposureControl ? pipelineToneMapAdaptive : pipelineToneMap });

    // ToneMap and generate LDR render target (Alpha channel contains pre-toneMapped luminance value)
    GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
    descriptorSetCmd._set._textureData.setTexture(data0, to_U8(ShaderProgram::TextureUsage::UNIT0));

    if (_adaptiveExposureControl) {
        TextureData data1 = _currentLuminance._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data();
        descriptorSetCmd._set._textureData.setTexture(data1, to_U8(ShaderProgram::TextureUsage::UNIT1));
    }
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::BeginRenderPassCommand beginRenderPassCmd = {};
    beginRenderPassCmd._target = _postFXOutput._targetID;
    beginRenderPassCmd._name = "DO_TONEMAP_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _toneMapConstants });
    GFX::EnqueueCommand(bufferInOut, GFX::DrawCommand{triangleCmd});
    GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

    // Execute all LDR based operators
    for (PreRenderOperator* op : ldrBatch) {
        if (op != nullptr && BitCompare(filterStack, to_U16(op->operatorType()))) {
            op->execute(camera, bufferInOut);
        }
    }
}

void PreRenderBatch::reshape(U16 width, U16 height) {
    // make the texture square sized and power of two
    U16 lumaRez = to_U16(nextPOW2(to_U32(width / 3.0f)));

    _currentLuminance._rt->resize(lumaRez, lumaRez);
    _previousLuminance._rt->resize(1, 1);

    _toneMapConstants.set(_ID("luminanceMipLevel"),
                          GFX::PushConstantType::INT,
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