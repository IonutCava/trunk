#include "stdafx.h"

#include "Headers/PreRenderBatch.h"
#include "Headers/PreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DofPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/PostAAPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"

namespace Divide {

namespace {
    //ToneMap ref: https://bruop.github.io/exposure/
    constexpr U16 GROUP_SIZE = 256u;
    constexpr U8  GROUP_X_THREADS = 16u;
    constexpr U8  GROUP_Y_THREADS = 16u;
};

PreRenderBatch::PreRenderBatch(GFXDevice& context, PostFX& parent, ResourceCache& cache, RenderTargetID renderTarget)
    : _context(context),
      _resCache(cache),
      _parent(parent),
      _renderTarget(renderTarget)
{
    _edgeDetectionPipelines.fill(nullptr);
    init();
}

PreRenderBatch::~PreRenderBatch()
{
    destroy();
}

void PreRenderBatch::destroy() {
    for (OperatorBatch& batch : _operators) {
        MemoryManager::DELETE_CONTAINER(batch);
    }
    
    _context.renderTargetPool().deallocateRT(_currentLuminance);
    _context.renderTargetPool().deallocateRT(_postFXOutput);
}

void PreRenderBatch::init() {
    assert(_postFXOutput._targetID._usage == RenderTargetUsage::COUNT);

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
        vectorSTD<RTAttachmentDescriptor> att = {
            { outputDescriptor, RTAttachmentType::Colour }
        };

        RenderTargetDescriptor desc = {};
        desc._name = "PostFXOutput";
        desc._resolution = rt.getResolution();
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _postFXOutput = _context.renderTargetPool().allocateRT(desc);
    }
    {
        SamplerDescriptor lumaSampler = {};
        lumaSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
        lumaSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
        lumaSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
        lumaSampler.minFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
        lumaSampler.magFilter(TextureFilter::LINEAR);

        TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RED, GFXDataFormat::FLOAT_16);
        lumaDescriptor.samplerDescriptor(lumaSampler);
        lumaDescriptor.autoMipMaps(true);
        vectorSTD<RTAttachmentDescriptor> att = {
            { lumaDescriptor, RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "Luminance";
        desc._resolution = vec2<U16>(1);
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _currentLuminance = _context.renderTargetPool().allocateRT(desc);
    }
    {
        SamplerDescriptor sampler = {};
        sampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
        sampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
        sampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
        sampler.minFilter(TextureFilter::LINEAR);
        sampler.magFilter(TextureFilter::LINEAR);
        sampler.anisotropyLevel(0);

        TextureDescriptor edgeDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RG, GFXDataFormat::FLOAT_16);
        edgeDescriptor.samplerDescriptor(sampler);

        vectorSTD<RTAttachmentDescriptor> att = {
            { edgeDescriptor, RTAttachmentType::Colour }
        };

        RenderTargetDescriptor desc = {};
        desc._name = "SceneEdges";
        desc._resolution = rt.getResolution();
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _sceneEdges = _context.renderTargetPool().allocateRT(desc);
    }

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
        ShaderModuleDescriptor computeModule = {};
        computeModule._moduleType = ShaderType::COMPUTE;
        computeModule._sourceFile = "luminanceCalc.glsl";
        computeModule._defines.emplace_back(Util::StringFormat("GROUP_SIZE %d", GROUP_SIZE), true);
        computeModule._defines.emplace_back(Util::StringFormat("THREADS_X %d", GROUP_X_THREADS), true);
        computeModule._defines.emplace_back(Util::StringFormat("THREADS_Y %d", GROUP_Y_THREADS), true);

        {
            computeModule._variant = "Create";
            ShaderProgramDescriptor calcDescriptor = {};
            calcDescriptor._modules.push_back(computeModule);

            ResourceDescriptor histogramCreate("luminanceCalc.HistogramCreate");
            histogramCreate.threaded(false);
            histogramCreate.propertyDescriptor(calcDescriptor);
            _createHistogram = CreateResource<ShaderProgram>(_resCache, histogramCreate);
        }
        {
            computeModule._variant = "Average";
            ShaderProgramDescriptor calcDescriptor = {};
            calcDescriptor._modules.push_back(computeModule);

            ResourceDescriptor histogramAverage("luminanceCalc.HistogramAverage");
            histogramAverage.threaded(false);
            histogramAverage.propertyDescriptor(calcDescriptor);
            _averageHistogram = CreateResource<ShaderProgram>(_resCache, histogramAverage);
        }
    }
    {
        ShaderProgramDescriptor edgeDetectionDescriptor = {};
        ShaderModuleDescriptor vertModule = {};
        vertModule._moduleType = ShaderType::VERTEX;
        vertModule._sourceFile = "EdgeDetection.glsl";

        ShaderModuleDescriptor fragModule = {};
        fragModule._moduleType = ShaderType::FRAGMENT;
        fragModule._sourceFile = "EdgeDetection.glsl";

        {
            fragModule._variant = "Depth";
            edgeDetectionDescriptor._modules = { vertModule, fragModule };

            ResourceDescriptor edgeDetectionDepth("edgeDetection.Depth");
            edgeDetectionDepth.threaded(false);
            edgeDetectionDepth.propertyDescriptor(edgeDetectionDescriptor);

            _edgeDetection[to_base(EdgeDetectionMethod::Depth)] = CreateResource<ShaderProgram>(_resCache, edgeDetectionDepth);
        }
        {
            fragModule._variant = "Luma";
            edgeDetectionDescriptor._modules = { vertModule, fragModule };

            ResourceDescriptor edgeDetectionLuma("edgeDetection.Luma");
            edgeDetectionLuma.threaded(false);
            edgeDetectionLuma.propertyDescriptor(edgeDetectionDescriptor);
            _edgeDetection[to_base(EdgeDetectionMethod::Luma)] = CreateResource<ShaderProgram>(_resCache, edgeDetectionLuma);
        }
        {
            fragModule._variant = "Colour";
            edgeDetectionDescriptor._modules = { vertModule, fragModule };

            ResourceDescriptor edgeDetectionColour("edgeDetection.Colour");
            edgeDetectionColour.threaded(false);
            edgeDetectionColour.propertyDescriptor(edgeDetectionDescriptor);
            _edgeDetection[to_base(EdgeDetectionMethod::Colour)] = CreateResource<ShaderProgram>(_resCache, edgeDetectionColour);
        }
    }

    ShaderBufferDescriptor bufferDescriptor = {};
    bufferDescriptor._name = "LUMINANCE_HISTOGRAM_BUFFER";
    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._elementCount = 256;
    bufferDescriptor._elementSize = sizeof(U32);
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
    bufferDescriptor._updateUsage = BufferUpdateUsage::GPU_R_GPU_W;
    bufferDescriptor._ringBufferLength = 0;
    bufferDescriptor._separateReadWrite = false;

    _histogramBuffer = _context.newSB(bufferDescriptor);
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

void PreRenderBatch::update(const U64 deltaTimeUS) {
    _lastDeltaTimeUS = deltaTimeUS;
}

RenderTargetHandle PreRenderBatch::inputRT() const noexcept {
    return RenderTargetHandle(_renderTarget, &_context.renderTargetPool().renderTarget(_renderTarget));
}

RenderTargetHandle PreRenderBatch::edgesRT() const noexcept {
    return _sceneEdges;
}

RenderTargetHandle PreRenderBatch::luminanceRT() const noexcept {
    return _currentLuminance;
}

RenderTargetHandle& PreRenderBatch::outputRT() noexcept {
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
    static Pipeline* pipelineLumCalcHistogram = nullptr, * pipelineLumCalcAverage = nullptr, * pipelineToneMap = nullptr, * pipelineToneMapAdaptive = nullptr;
    if (pipelineLumCalcHistogram == nullptr) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = _context.get2DStateBlock();

        pipelineDescriptor._shaderProgramHandle = _createHistogram->getGUID();
        pipelineLumCalcHistogram = _context.newPipeline(pipelineDescriptor);

        pipelineDescriptor._shaderProgramHandle = _averageHistogram->getGUID();
        pipelineLumCalcAverage = _context.newPipeline(pipelineDescriptor);

        pipelineDescriptor._shaderProgramHandle = _toneMapAdaptive->getGUID();
        pipelineToneMapAdaptive = _context.newPipeline(pipelineDescriptor);

        pipelineDescriptor._shaderProgramHandle = _toneMap->getGUID();
        pipelineToneMap = _context.newPipeline(pipelineDescriptor);

        for (U8 i = 0; i < to_U8(EdgeDetectionMethod::COUNT); ++i) {
            pipelineDescriptor._shaderProgramHandle = _edgeDetection[i]->getGUID();
            _edgeDetectionPipelines[i] = _context.newPipeline(pipelineDescriptor);
        }
    }

    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];

    GenericDrawCommand triangleCmd;
    triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
    triangleCmd._drawCount = 1;

    _toneMapParams.width = inputRT()._rt->getWidth();
    _toneMapParams.height = inputRT()._rt->getHeight();

    const Texture_ptr& screenColour = inputRT()._rt->getAttachment(RTAttachmentType::Colour, 0).texture();
    const Texture_ptr& screenDepth = inputRT()._rt->getAttachment(RTAttachmentType::Depth, 0).texture();
    const Texture_ptr& luminanceTex = _currentLuminance._rt->getAttachment(RTAttachmentType::Colour, 0).texture();

    if (adaptiveExposureControl()) {
        const F32 logLumRange = _toneMapParams.maxLogLuminance - _toneMapParams.minLogLuminance;
        const F32 histogramParams[4] = {
                _toneMapParams.minLogLuminance,
                1.0f / (logLumRange),
                to_F32(_toneMapParams.width),
                to_F32(_toneMapParams.height),
        };

        const U32 groupsX = to_U32(std::ceil(_toneMapParams.width / to_F32(GROUP_X_THREADS)));
        const U32 groupsY = to_U32(std::ceil(_toneMapParams.height / to_F32(GROUP_Y_THREADS)));

        ShaderBufferBinding shaderBuffer = {};
        shaderBuffer._binding = ShaderBufferLocation::LUMINANCE_HISTOGRAM;
        shaderBuffer._buffer = _histogramBuffer;
        shaderBuffer._elementRange = { 0u, _histogramBuffer->getPrimitiveCount() };

        {
            Image screenImage = {};
            screenImage._texture = screenColour.get();
            screenImage._flag = Image::Flag::READ;
            screenImage._binding = 0u;
            screenImage._layer = 0u;
            screenImage._level = 0u;

            GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
            beginDebugScopeCmd._scopeName = "CreateLuminanceHistogram";
            GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

            GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
            bindDescriptorSetsCmd._set.addShaderBuffer(shaderBuffer);
            bindDescriptorSetsCmd._set._images.push_back(screenImage);
            GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

            GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ pipelineLumCalcHistogram });

            GFX::SendPushConstantsCommand pushConstantsCommand = {};
            pushConstantsCommand._constants.set(_ID("u_params"), GFX::PushConstantType::VEC4, histogramParams);
            GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

            GFX::DispatchComputeCommand computeCmd = {};
            computeCmd._computeGroupSize.set(groupsX, groupsY, 1);
            GFX::EnqueueCommand(bufferInOut, computeCmd);

            GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
        }
        GFX::MemoryBarrierCommand memCmd = {};
        memCmd._barrierMask = to_base(MemoryBarrierType::BUFFER_UPDATE);
        GFX::EnqueueCommand(bufferInOut, memCmd);
        {
            const F32 deltaTime = Time::MicrosecondsToSeconds<F32>(_lastDeltaTimeUS);
            const F32 timeCoeff = CLAMPED_01(1.0f - std::exp(-deltaTime * _toneMapParams.tau));

            const F32 avgParams[4] = {
                _toneMapParams.minLogLuminance,
                logLumRange,
                timeCoeff,
                to_F32(to_U32(_toneMapParams.width) * _toneMapParams.height),
            };

            Image luminanceImage = {};
            luminanceImage._texture = luminanceTex.get();
            luminanceImage._flag = Image::Flag::READ_WRITE;
            luminanceImage._binding = 0u;
            luminanceImage._layer = 0u;
            luminanceImage._level = 0u;

            GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
            beginDebugScopeCmd._scopeName = "AverageLuminanceHistogram";
            GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

            GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
            bindDescriptorSetsCmd._set.addShaderBuffer(shaderBuffer);
            bindDescriptorSetsCmd._set._images.push_back(luminanceImage);
            GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

            GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ pipelineLumCalcAverage });

            GFX::SendPushConstantsCommand pushConstantsCommand = {};
            pushConstantsCommand._constants.set(_ID("u_params"), GFX::PushConstantType::VEC4, avgParams);
            GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

            GFX::DispatchComputeCommand computeCmd = {};
            computeCmd._computeGroupSize.set(1, 1, 1);
            GFX::EnqueueCommand(bufferInOut, computeCmd);

            GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
        }

        memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_IMAGE);
        GFX::EnqueueCommand(bufferInOut, memCmd);
    }

    // Execute all HDR based operators
    for (PreRenderOperator* op : hdrBatch) {
        if (op != nullptr && BitCompare(filterStack, to_U16(op->operatorType()))) {
            op->execute(camera, bufferInOut);
        }
    }

    GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
    descriptorSetCmd._set._textureData.setTexture(screenColour->data(), to_U8(ShaderProgram::TextureUsage::UNIT0));
    descriptorSetCmd._set._textureData.setTexture(luminanceTex->data(), to_U8(ShaderProgram::TextureUsage::UNIT1));
    descriptorSetCmd._set._textureData.setTexture(screenDepth->data(), to_U8(ShaderProgram::TextureUsage::DEPTH));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    // ToneMap and generate LDR render target (Alpha channel contains pre-toneMapped luminance value)
    {
        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        beginRenderPassCmd._target = _postFXOutput._targetID;
        beginRenderPassCmd._name = "DO_TONEMAP_PASS";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ adaptiveExposureControl() ? pipelineToneMapAdaptive : pipelineToneMap });

        _toneMapConstants.set(_ID("exposure"), GFX::PushConstantType::FLOAT, adaptiveExposureControl() ? _toneMapParams.manualExposureAdaptive : _toneMapParams.manualExposure);
        _toneMapConstants.set(_ID("whitePoint"), GFX::PushConstantType::FLOAT, _toneMapParams.manualWhitePoint);
        GFX::EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _toneMapConstants });

        GFX::EnqueueCommand(bufferInOut, GFX::DrawCommand{ triangleCmd });
        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
    }

    // Now that we have an LDR target, proceed with edge detection
    if (edgeDetectionMethod() != EdgeDetectionMethod::COUNT) {
        RTClearDescriptor clearTarget = {};
        clearTarget.clearColours(true);

        GFX::ClearRenderTargetCommand clearEdgeTarget = {};
        clearEdgeTarget._target = _sceneEdges._targetID;
        clearEdgeTarget._descriptor = clearTarget;
        GFX::EnqueueCommand(bufferInOut, clearEdgeTarget);

        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        beginRenderPassCmd._target = _sceneEdges._targetID;
        beginRenderPassCmd._name = "DO_EDGE_DETECT_PASS";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _edgeDetectionPipelines[to_base(edgeDetectionMethod())] });
        
        GFX::SendPushConstantsCommand pushConstantsCommand = {};
        pushConstantsCommand._constants.set(_ID("dvd_edgeThreshold"), GFX::PushConstantType::FLOAT, edgeDetectionThreshold());
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::EnqueueCommand(bufferInOut, GFX::DrawCommand{ triangleCmd });
        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
    }

    // Execute all LDR based operators
    for (PreRenderOperator* op : ldrBatch) {
        if (op != nullptr && BitCompare(filterStack, to_U16(op->operatorType()))) {
            op->execute(camera, bufferInOut);
        }
    }
}

void PreRenderBatch::reshape(U16 width, U16 height) {
    // make the texture square sized and power of two
    const U16 lumaRez = to_U16(nextPOW2(to_U32(width / 3.0f)));

    for (OperatorBatch& batch : _operators) {
        for (PreRenderOperator* op : batch) {
            if (op != nullptr) {
                op->reshape(width, height);
            }
        }
    }

    _postFXOutput._rt->resize(width, height);
    _sceneEdges._rt->resize(width, height);
}
};