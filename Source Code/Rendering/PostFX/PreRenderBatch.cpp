#include "stdafx.h"

#include "Headers/PreRenderBatch.h"
#include "Headers/PreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/MotionBlurPreRenderOperator.h"
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

PreRenderBatch::PreRenderBatch(GFXDevice& context, PostFX& parent, ResourceCache* cache)
    : _context(context),
      _resCache(cache),
      _parent(parent)
{
    std::atomic_uint loadTasks = 0;

    // We only work with the resolved screen target
    _screenRTs._hdr._screenRef._targetID = RenderTargetID(RenderTargetUsage::SCREEN);
    _screenRTs._hdr._screenRef._rt = &context.renderTargetPool().renderTarget(_screenRTs._hdr._screenRef._targetID);

    SamplerDescriptor screenSampler = {};
    screenSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.minFilter(TextureFilter::LINEAR);
    screenSampler.magFilter(TextureFilter::LINEAR);
    screenSampler.anisotropyLevel(0);

    RenderTargetDescriptor desc = {};
    desc._resolution = _screenRTs._hdr._screenRef._rt->getResolution();
    desc._attachmentCount = 1u;

    TextureDescriptor outputDescriptor = _screenRTs._hdr._screenRef._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ALBEDO)).texture()->descriptor();
    outputDescriptor.samplerDescriptor(screenSampler);

    {
        vectorEASTL<RTAttachmentDescriptor> att = { { outputDescriptor, RTAttachmentType::Colour } };
        desc._name = "PostFX Output HDR";
        desc._attachments = att.data();

        _screenRTs._hdr._screenCopy = _context.renderTargetPool().allocateRT(desc);
    }
    {
        outputDescriptor.dataType(GFXDataFormat::UNSIGNED_BYTE);
        //Colour0 holds the LDR screen texture
        vectorEASTL<RTAttachmentDescriptor> att = { { outputDescriptor, RTAttachmentType::Colour } };

        desc._name = "PostFX Output LDR 0";
        desc._attachments = att.data();

        _screenRTs._ldr._temp[0] = _context.renderTargetPool().allocateRT(desc);

        desc._name = "PostFX Output LDR 1";
        _screenRTs._ldr._temp[1] = _context.renderTargetPool().allocateRT(desc);
    }
    {
        TextureDescriptor edgeDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RG, GFXDataFormat::FLOAT_16);
        edgeDescriptor.samplerDescriptor(screenSampler);
        vectorEASTL<RTAttachmentDescriptor> att = { { edgeDescriptor, RTAttachmentType::Colour } };

        desc._name = "SceneEdges";
        desc._attachments = att.data();
        _sceneEdges = _context.renderTargetPool().allocateRT(desc);
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
        vectorEASTL<RTAttachmentDescriptor> att = { { lumaDescriptor, RTAttachmentType::Colour }, };

        desc._name = "Luminance";
        desc._resolution = vec2<U16>(1);
        desc._attachments = att.data();
        _currentLuminance = _context.renderTargetPool().allocateRT(desc);
    }

    // Order is very important!
    OperatorBatch& hdrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_HDR)];
    for (U16 i = 1; i < to_base(FilterType::FILTER_COUNT); ++i) {
        const FilterType fType = static_cast<FilterType>(toBit(i));

        if (getOperatorSpace(fType) == FilterSpace::FILTER_SPACE_HDR) {
            switch (fType) {
                case FilterType::FILTER_SS_AMBIENT_OCCLUSION:
                    hdrBatch.emplace_back(eastl::make_unique<SSAOPreRenderOperator>(_context, *this, _resCache));
                    break;

                case FilterType::FILTER_SS_REFLECTIONS:
                    break;

                case FilterType::FILTER_DEPTH_OF_FIELD:
                    hdrBatch.emplace_back(eastl::make_unique<DoFPreRenderOperator>(_context, *this, _resCache));
                    break;

                case FilterType::FILTER_MOTION_BLUR:
                    hdrBatch.emplace_back(eastl::make_unique<MotionBlurPreRenderOperator>(_context, *this, _resCache));
                    break;

                case FilterType::FILTER_BLOOM:
                    hdrBatch.emplace_back(eastl::make_unique<BloomPreRenderOperator>(_context, *this, _resCache));
                    break;

                default:
                    DIVIDE_UNEXPECTED_CALL();
                    break;
            }
        }
    }

    OperatorBatch& ldrBatch = _operators[to_base(FilterSpace::FILTER_SPACE_LDR)];
    for (U16 i = 1; i < to_base(FilterType::FILTER_COUNT); ++i) {
        const FilterType fType = static_cast<FilterType>(toBit(i));

        if (getOperatorSpace(fType) == FilterSpace::FILTER_SPACE_LDR) {
            switch (fType) {
                case FilterType::FILTER_SS_ANTIALIASING:
                    ldrBatch.push_back(eastl::make_unique<PostAAPreRenderOperator>(_context, *this, _resCache));
                    break;
                default:
                    DIVIDE_UNEXPECTED_CALL();
                    break;
            }
        }
    }
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
        toneMap.waitForReady(false);
        toneMap.propertyDescriptor(mapDescriptor1);
        _toneMap = CreateResource<ShaderProgram>(_resCache, toneMap, loadTasks);

        fragModule._defines.emplace_back("USE_ADAPTIVE_LUMINANCE", true);

        ShaderProgramDescriptor mapDescriptor2 = {};
        mapDescriptor2._modules.push_back(vertModule);
        mapDescriptor2._modules.push_back(fragModule);

        ShaderProgramDescriptor toneMapAdaptiveDescriptor;
        toneMapAdaptiveDescriptor._modules.push_back(vertModule);
        toneMapAdaptiveDescriptor._modules.push_back(fragModule);

        ResourceDescriptor toneMapAdaptive("toneMap.Adaptive");
        toneMapAdaptive.waitForReady(false);
        toneMapAdaptive.propertyDescriptor(toneMapAdaptiveDescriptor);

        _toneMapAdaptive = CreateResource<ShaderProgram>(_resCache, toneMapAdaptive, loadTasks);
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
            histogramCreate.waitForReady(false);
            histogramCreate.propertyDescriptor(calcDescriptor);
            _createHistogram = CreateResource<ShaderProgram>(_resCache, histogramCreate, loadTasks);
        }
        {
            computeModule._variant = "Average";
            ShaderProgramDescriptor calcDescriptor = {};
            calcDescriptor._modules.push_back(computeModule);

            ResourceDescriptor histogramAverage("luminanceCalc.HistogramAverage");
            histogramAverage.waitForReady(false);
            histogramAverage.propertyDescriptor(calcDescriptor);
            _averageHistogram = CreateResource<ShaderProgram>(_resCache, histogramAverage, loadTasks);
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
            edgeDetectionDepth.waitForReady(false);
            edgeDetectionDepth.propertyDescriptor(edgeDetectionDescriptor);

            _edgeDetection[to_base(EdgeDetectionMethod::Depth)] = CreateResource<ShaderProgram>(_resCache, edgeDetectionDepth, loadTasks);
        }
        {
            fragModule._variant = "Luma";
            edgeDetectionDescriptor._modules = { vertModule, fragModule };

            ResourceDescriptor edgeDetectionLuma("edgeDetection.Luma");
            edgeDetectionLuma.waitForReady(false);
            edgeDetectionLuma.propertyDescriptor(edgeDetectionDescriptor);
            _edgeDetection[to_base(EdgeDetectionMethod::Luma)] = CreateResource<ShaderProgram>(_resCache, edgeDetectionLuma, loadTasks);

        }
        {
            fragModule._variant = "Colour";
            edgeDetectionDescriptor._modules = { vertModule, fragModule };

            ResourceDescriptor edgeDetectionColour("edgeDetection.Colour");
            edgeDetectionColour.waitForReady(false);
            edgeDetectionColour.propertyDescriptor(edgeDetectionDescriptor);
            _edgeDetection[to_base(EdgeDetectionMethod::Colour)] = CreateResource<ShaderProgram>(_resCache, edgeDetectionColour, loadTasks);
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

    WAIT_FOR_CONDITION(operatorsReady());
    WAIT_FOR_CONDITION(loadTasks.load() == 0);
}

PreRenderBatch::~PreRenderBatch()
{
    _context.renderTargetPool().deallocateRT(_currentLuminance);
    _context.renderTargetPool().deallocateRT(_screenRTs._hdr._screenCopy);
    _context.renderTargetPool().deallocateRT(_screenRTs._ldr._temp[0]);
    _context.renderTargetPool().deallocateRT(_screenRTs._ldr._temp[1]);
}

bool PreRenderBatch::operatorsReady() const {
    for (const OperatorBatch& batch : _operators) {
        for (const auto& op : batch) {
            if (!op->ready()) {
                return false;
            }
        }
    }

    return true;
}

RenderTargetHandle PreRenderBatch::getInput(bool hdr) const {
    if (hdr && _swapped) {
        return _screenRTs._hdr._screenCopy;
    } else if (hdr) {
        return _screenRTs._hdr._screenRef;
    }

    return _screenRTs._ldr._temp[_swapped ? 0 : 1];
}

RenderTargetHandle PreRenderBatch::getOutput(bool hdr) const {
    if (hdr && _swapped) {
        return _screenRTs._hdr._screenRef;
    } else if (hdr) {
        return _screenRTs._hdr._screenCopy;
    }

    return _screenRTs._ldr._temp[_swapped ? 1 : 0];
}

void PreRenderBatch::idle(const Configuration& config) {
    for (OperatorBatch& batch : _operators) {
        for (auto& op : batch) {
            op->idle(config);
        }
    }
}

void PreRenderBatch::update(const U64 deltaTimeUS) {
    _lastDeltaTimeUS = deltaTimeUS;
    _swapped = false;
}

RenderTargetHandle PreRenderBatch::screenRT() const noexcept {
    return _screenRTs._hdr._screenRef;
}

RenderTargetHandle PreRenderBatch::edgesRT() const noexcept {
    return _sceneEdges;
}

RenderTargetHandle PreRenderBatch::luminanceRT() const noexcept {
    return _currentLuminance;
}

void PreRenderBatch::onFilterEnabled(FilterType filter) {
    onFilterToggle(filter, true);
}

void PreRenderBatch::onFilterDisabled(FilterType filter) {
    onFilterToggle(filter, false);
}

void PreRenderBatch::onFilterToggle(FilterType filter, const bool state) {
    for (OperatorBatch& batch : _operators) {
        for (auto& op : batch) {
            if (filter == op->operatorType()) {
                op->onToggle(state);
            }
        }
    }
}

void PreRenderBatch::prepare(const Camera& camera, U32 filterStack, GFX::CommandBuffer& bufferInOut) {
    for (OperatorBatch& batch : _operators) {
        for (auto& op : batch) {
            op->prepare(camera, bufferInOut);
        }
    }
}

void PreRenderBatch::execute(const Camera& camera, U32 filterStack, GFX::CommandBuffer& bufferInOut) {
    static Pipeline* pipelineLumCalcHistogram = nullptr, * pipelineLumCalcAverage = nullptr, * pipelineToneMap = nullptr, * pipelineToneMapAdaptive = nullptr;
    static GFX::DrawCommand drawCmd = { GenericDrawCommand { PrimitiveType::TRIANGLES } };

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

    _toneMapParams.width = screenRT()._rt->getWidth();
    _toneMapParams.height = screenRT()._rt->getHeight();

    const Texture_ptr& screenColour = screenRT()._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ALBEDO)).texture();
    const Texture_ptr& screenDepth = screenRT()._rt->getAttachment(RTAttachmentType::Depth, 0).texture();
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

            GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "CreateLuminanceHistogram" });

            GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
            bindDescriptorSetsCmd._set.addShaderBuffer(shaderBuffer);
            bindDescriptorSetsCmd._set._images.push_back(screenImage);
            GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

            GFX::BindPipelineCommand pipelineCmd = {};
            pipelineCmd._pipeline = pipelineLumCalcHistogram;
            GFX::EnqueueCommand(bufferInOut, pipelineCmd);

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

            GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "AverageLuminanceHistogram" });

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
    for (auto& op : hdrBatch) {
        if (BitCompare(filterStack, to_U32(op->operatorType()))) {
            if (op->execute(camera, getInput(true), getOutput(true), bufferInOut)) {
                _swapped = !_swapped;
            }
        }
    }

    // ToneMap and generate LDR render target (Alpha channel contains pre-toneMapped luminance value)
    {
        const TextureData screenTex = getInput(true)._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ALBEDO)).texture()->data();

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set._textureData.setTexture(screenTex, TextureUsage::UNIT0);
        descriptorSetCmd._set._textureData.setTexture(luminanceTex->data(), TextureUsage::UNIT1);
        descriptorSetCmd._set._textureData.setTexture(screenDepth->data(), TextureUsage::DEPTH);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        beginRenderPassCmd._target = getOutput(false)._targetID;
        beginRenderPassCmd._name = "DO_TONEMAP_PASS";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ adaptiveExposureControl() ? pipelineToneMapAdaptive : pipelineToneMap });

        _toneMapConstants.set(_ID("exposure"), GFX::PushConstantType::FLOAT, adaptiveExposureControl() ? _toneMapParams.manualExposureAdaptive : _toneMapParams.manualExposure);
        _toneMapConstants.set(_ID("whitePoint"), GFX::PushConstantType::FLOAT, _toneMapParams.manualWhitePoint);
        GFX::EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _toneMapConstants });

        GFX::EnqueueCommand(bufferInOut, drawCmd);
        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

        // We need input to be LDR after this step
        _swapped = !_swapped;
    }

    // Now that we have an LDR target, proceed with edge detection
    if (edgeDetectionMethod() != EdgeDetectionMethod::COUNT) {
        const TextureData screenTex = getInput(false)._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ALBEDO)).texture()->data();

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set._textureData.setTexture(screenTex, TextureUsage::UNIT0);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

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

        GFX::EnqueueCommand(bufferInOut, drawCmd);
        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
    }
    
    // Execute all LDR based operators
    for (auto& op : ldrBatch) {
        if (BitCompare(filterStack, to_U32(op->operatorType()))) {
            if (op->execute(camera, getInput(false), getOutput(false), bufferInOut)) {
                _swapped = !_swapped;
            }
        }
    }

    // At this point, the last output should remain the general output. So the last swap was redundant
    _swapped = !_swapped;
}

void PreRenderBatch::reshape(U16 width, U16 height) {
    // make the texture square sized and power of two
    const U16 lumaRez = to_U16(nextPOW2(to_U32(width / 3.0f)));

    for (OperatorBatch& batch : _operators) {
        for (auto& op : batch) {
            op->reshape(width, height);
        }
    }

    _screenRTs._hdr._screenCopy._rt->resize(width, height);
    _screenRTs._ldr._temp[0]._rt->resize(width, height);
    _screenRTs._ldr._temp[1]._rt->resize(width, height);
    _sceneEdges._rt->resize(width, height);
}
};