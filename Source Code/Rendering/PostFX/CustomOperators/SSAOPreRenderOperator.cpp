#include "stdafx.h"

#include "Headers/SSAOPreRenderOperator.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Rendering/PostFX/Headers/PreRenderBatch.h"

namespace Divide {

namespace {
    constexpr U8 SSAO_NOISE_SIZE = 4;
    constexpr U8 MAX_KERNEL_SIZE = 64;
    std::array<vec3<F32>, MAX_KERNEL_SIZE> g_kernel;
}

//ref: http://john-chapman-graphics.blogspot.co.uk/2013/01/ssao-tutorial.html
SSAOPreRenderOperator::SSAOPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache* cache)
    : PreRenderOperator(context, parent, FilterType::FILTER_SS_AMBIENT_OCCLUSION) {
    _genHalfRes = context.context().config().rendering.postFX.ssao.UseHalfResolution;

    _kernelSampleCount[0] = context.context().config().rendering.postFX.ssao.FullRes.KernelSampleCount;
    _kernelSampleCount[1] = context.context().config().rendering.postFX.ssao.HalfRes.KernelSampleCount;
    _blur[0] = context.context().config().rendering.postFX.ssao.FullRes.Blur;
    _blur[1] = context.context().config().rendering.postFX.ssao.HalfRes.Blur;
    _radius[0] = context.context().config().rendering.postFX.ssao.FullRes.Radius;
    _radius[1] = context.context().config().rendering.postFX.ssao.HalfRes.Radius;
    _power[0] = context.context().config().rendering.postFX.ssao.FullRes.Power;
    _power[1] = context.context().config().rendering.postFX.ssao.HalfRes.Power;
    _bias[0] = context.context().config().rendering.postFX.ssao.FullRes.Bias;
    _bias[1] = context.context().config().rendering.postFX.ssao.HalfRes.Bias;

    vectorEASTL<vec3<F32>> noiseData(SSAO_NOISE_SIZE * SSAO_NOISE_SIZE);

    for (vec3<F32>& noise : noiseData) {
        noise.set(Random(-1.0f, 1.0f),
                  Random(-1.0f, 1.0f),
                  0.0f);
    }

    for (U16 i = 0; i < MAX_KERNEL_SIZE; ++i) {
        vec3<F32>& k = g_kernel[i];
        k.set(Random(-1.0f, 1.0f),
              Random(-1.0f, 1.0f),
              Random(0.0f, 1.0f));
        k.normalize();
        const F32 scaleSq = SQUARED(to_F32(i) / to_F32(MAX_KERNEL_SIZE));
        k *= Lerp(0.1f, 1.0f, scaleSq);
    }

    SamplerDescriptor nearestSampler = {};
    nearestSampler.wrapUVW(TextureWrap::CLAMP_TO_EDGE);
    nearestSampler.minFilter(TextureFilter::NEAREST);
    nearestSampler.magFilter(TextureFilter::NEAREST);
    nearestSampler.anisotropyLevel(0);

    SamplerDescriptor screenSampler = {};
    screenSampler.wrapUVW(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.minFilter(TextureFilter::LINEAR);
    screenSampler.magFilter(TextureFilter::LINEAR);
    screenSampler.anisotropyLevel(0);

    SamplerDescriptor noiseSampler = {};
    noiseSampler.wrapUVW(TextureWrap::REPEAT);
    noiseSampler.minFilter(TextureFilter::NEAREST);
    noiseSampler.magFilter(TextureFilter::NEAREST);
    noiseSampler.anisotropyLevel(0);
    _noiseSampler = noiseSampler.getHash();

    Str64 attachmentName("SSAOPreRenderOperator_NoiseTexture");

    TextureDescriptor noiseDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGB, GFXDataFormat::FLOAT_32);
    noiseDescriptor.mipCount(1u);

    ResourceDescriptor textureAttachment(attachmentName);
    textureAttachment.propertyDescriptor(noiseDescriptor);
    _noiseTexture = CreateResource<Texture>(cache, textureAttachment);

    _noiseTexture->loadData({ (Byte*)noiseData.data(), noiseData.size() * sizeof(FColour3) }, vec2<U16>(SSAO_NOISE_SIZE));

    {

        TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RED, GFXDataFormat::FLOAT_16);
        outputDescriptor.mipCount(1u);

        RTAttachmentDescriptors att = {
            { outputDescriptor, nearestSampler.getHash(), RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "SSAO_Out";
        desc._resolution = parent.screenRT()._rt->getResolution();
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        //Colour0 holds the AO texture
        _ssaoOutput = _context.renderTargetPool().allocateRT(desc);

        desc._name = "SSAO_Out_HalfRes";
        desc._resolution /= 2;

        _ssaoHalfResOutput = _context.renderTargetPool().allocateRT(desc);
    }

    {
        TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D, GFXImageFormat::RGBA, GFXDataFormat::FLOAT_32);
        outputDescriptor.mipCount(1u);

        RTAttachmentDescriptors att = {
            { outputDescriptor, nearestSampler.getHash(), RTAttachmentType::Colour },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "HalfRes_Normals_Depth";
        desc._resolution = parent.screenRT()._rt->getResolution() / 2;
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        //Colour0 holds the AO texture
        _halfDepthAndNormals = _context.renderTargetPool().allocateRT(desc);
    }

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "FullScreenQuad";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "SSAOPass.glsl";

    { //Calc Full
        fragModule._variant = "SSAOCalc";
        fragModule._defines.resize(0);
        fragModule._defines.emplace_back(Util::StringFormat("MAX_KERNEL_SIZE %d", MAX_KERNEL_SIZE).c_str(), true);
        fragModule._defines.emplace_back(Util::StringFormat("SSAO_SAMPLE_COUNT %d", _kernelSampleCount[0]).c_str(), true);

        ShaderProgramDescriptor ssaoShaderDescriptor = {};
        ssaoShaderDescriptor._modules.push_back(vertModule);
        ssaoShaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor ssaoGenerate("SSAOCalc");
        ssaoGenerate.propertyDescriptor(ssaoShaderDescriptor);
        ssaoGenerate.waitForReady(false);
        _ssaoGenerateShader = CreateResource<ShaderProgram>(cache, ssaoGenerate);
    }
    { //Calc Half
        fragModule._variant = "SSAOCalc";
        fragModule._defines.resize(0);
        fragModule._defines.emplace_back(Util::StringFormat("MAX_KERNEL_SIZE %d", MAX_KERNEL_SIZE).c_str(), true);
        fragModule._defines.emplace_back(Util::StringFormat("SSAO_SAMPLE_COUNT %d", _kernelSampleCount[1]).c_str(), true);
        fragModule._defines.emplace_back("COMPUTE_HALF_RES", true);

        ShaderProgramDescriptor ssaoShaderDescriptor = {};
        ssaoShaderDescriptor._modules.push_back(vertModule);
        ssaoShaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor ssaoGenerateHalfRes("SSAOCalcHalfRes");
        ssaoGenerateHalfRes.propertyDescriptor(ssaoShaderDescriptor);
        ssaoGenerateHalfRes.waitForReady(false);
        _ssaoGenerateHalfResShader = CreateResource<ShaderProgram>(cache, ssaoGenerateHalfRes);
    }

    { //Blur
        fragModule._variant = "SSAOBlur";
        fragModule._defines.resize(0);
        fragModule._defines.emplace_back(Util::StringFormat("BLUR_SIZE %d", SSAO_NOISE_SIZE).c_str(), true);

        ShaderProgramDescriptor ssaoShaderDescriptor = {};
        ssaoShaderDescriptor._modules.push_back(vertModule);
        ssaoShaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor ssaoBlur("SSAOBlur");
        ssaoBlur.propertyDescriptor(ssaoShaderDescriptor);
        ssaoBlur.waitForReady(false);
        _ssaoBlurShader = CreateResource<ShaderProgram>(cache, ssaoBlur);
    }
    { //Pass-through
        fragModule._variant = "SSAOPassThrough";
        fragModule._defines.resize(0);

        ShaderProgramDescriptor ssaoShaderDescriptor  = {};
        ssaoShaderDescriptor._modules.push_back(vertModule);
        ssaoShaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor ssaoPassThrough("SSAOPAssThrough");
        ssaoPassThrough.propertyDescriptor(ssaoShaderDescriptor);
        ssaoPassThrough.waitForReady(false);

        _ssaoPassThroughShader = CreateResource<ShaderProgram>(cache, ssaoPassThrough);
    }
    { //DownSample
        fragModule._variant = "SSAODownsample";
        fragModule._defines.resize(0);

        ShaderProgramDescriptor ssaoShaderDescriptor = {};
        ssaoShaderDescriptor._modules.push_back(vertModule);
        ssaoShaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor ssaoDownSample("SSAODownSample");
        ssaoDownSample.propertyDescriptor(ssaoShaderDescriptor);
        ssaoDownSample.waitForReady(false);

        _ssaoDownSampleShader = CreateResource<ShaderProgram>(cache, ssaoDownSample);
    }
    { //UpSample
        fragModule._variant = "SSAOUpsample";
        fragModule._defines.resize(0);

        ShaderProgramDescriptor ssaoShaderDescriptor = {};
        ssaoShaderDescriptor._modules.push_back(vertModule);
        ssaoShaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor ssaoUpSample("SSAOUpSample");
        ssaoUpSample.propertyDescriptor(ssaoShaderDescriptor);
        ssaoUpSample.waitForReady(false);

        _ssaoUpSampleShader = CreateResource<ShaderProgram>(cache, ssaoUpSample);
    }

    _ssaoGenerateConstants.set(_ID("sampleKernel"), GFX::PushConstantType::VEC3, g_kernel);
    _ssaoGenerateConstants.set(_ID("SSAO_RADIUS"), GFX::PushConstantType::FLOAT, radius());
    _ssaoGenerateConstants.set(_ID("SSAO_INTENSITY"), GFX::PushConstantType::FLOAT, power());
    _ssaoGenerateConstants.set(_ID("SSAO_BIAS"), GFX::PushConstantType::FLOAT, bias());
}

SSAOPreRenderOperator::~SSAOPreRenderOperator() 
{
    _context.renderTargetPool().deallocateRT(_ssaoOutput);
    _context.renderTargetPool().deallocateRT(_halfDepthAndNormals);
    _context.renderTargetPool().deallocateRT(_ssaoHalfResOutput);
}

bool SSAOPreRenderOperator::ready() const {
    if (_ssaoBlurShader->getState() == ResourceState::RES_LOADED && 
        _ssaoGenerateShader->getState() == ResourceState::RES_LOADED && 
        _ssaoGenerateHalfResShader->getState() == ResourceState::RES_LOADED &&
        _ssaoDownSampleShader->getState() == ResourceState::RES_LOADED &&
        _ssaoPassThroughShader->getState() == ResourceState::RES_LOADED &&
        _ssaoUpSampleShader->getState() == ResourceState::RES_LOADED)
    {
        return PreRenderOperator::ready();
    }

    return false;
}

void SSAOPreRenderOperator::reshape(const U16 width, const U16 height) {
    PreRenderOperator::reshape(width, height);

    _ssaoOutput._rt->resize(width, height);
    _halfDepthAndNormals._rt->resize(width / 2, height / 2);

    const vec2<F32> targetDim = vec2<F32>(width, height) * (_genHalfRes ? 0.5f : 1.f);

    _ssaoGenerateConstants.set(_ID("SSAO_NOISE_SCALE"), GFX::PushConstantType::VEC2, targetDim  / _noiseTexture->width());
}

void SSAOPreRenderOperator::genHalfRes(const bool state) {
    if (_genHalfRes != state) {
        _genHalfRes = state;
        const U16 width = state ? _halfDepthAndNormals._rt->getWidth() : _ssaoOutput._rt->getWidth();
        const U16 height = state ? _halfDepthAndNormals._rt->getHeight() : _ssaoOutput._rt->getHeight();

        const vec2<F32> targetDim(width, height);

        _ssaoGenerateConstants.set(_ID("SSAO_NOISE_SCALE"), GFX::PushConstantType::VEC2, targetDim / _noiseTexture->width());
        _context.context().config().rendering.postFX.ssao.UseHalfResolution = state;
        _context.context().config().changed(true);
    }
}

void SSAOPreRenderOperator::radius(const F32 val) {
    if (!COMPARE(_radius[_genHalfRes ? 1 : 0], val)) {
        _radius[_genHalfRes ? 1 : 0] = val;
        _ssaoGenerateConstants.set(_ID("SSAO_RADIUS"), GFX::PushConstantType::FLOAT, val);
        if (_genHalfRes) {
            _context.context().config().rendering.postFX.ssao.HalfRes.Radius = val;
        } else {
            _context.context().config().rendering.postFX.ssao.FullRes.Radius = val;
        }
        _context.context().config().changed(true);
    }
}

void SSAOPreRenderOperator::power(const F32 val) {
    if (!COMPARE(_power[_genHalfRes ? 1 : 0], val)) {
        _power[_genHalfRes ? 1 : 0] = val;
        _ssaoGenerateConstants.set(_ID("SSAO_INTENSITY"), GFX::PushConstantType::FLOAT, val);
        if (_genHalfRes) {
            _context.context().config().rendering.postFX.ssao.HalfRes.Power = val;
        } else {
            _context.context().config().rendering.postFX.ssao.FullRes.Power = val;
        }
        _context.context().config().changed(true);
    }
}

void SSAOPreRenderOperator::bias(const F32 val) {
    if (!COMPARE(_bias[_genHalfRes ? 1 : 0], val)) {
        _bias[_genHalfRes ? 1 : 0] = val;
        _ssaoGenerateConstants.set(_ID("SSAO_BIAS"), GFX::PushConstantType::FLOAT, val);
        if (_genHalfRes) {
            _context.context().config().rendering.postFX.ssao.HalfRes.Bias = val;
        } else {
            _context.context().config().rendering.postFX.ssao.FullRes.Bias = val;
        }
        _context.context().config().changed(true);
    }
}

void SSAOPreRenderOperator::blurResults(const bool state) {
    if (_blur[_genHalfRes ? 1 : 0] != state) {
        _blur[_genHalfRes ? 1 : 0] = state;
        if (_genHalfRes) {
            _context.context().config().rendering.postFX.ssao.HalfRes.Blur = state;
        } else {
            _context.context().config().rendering.postFX.ssao.FullRes.Blur = state;
        }
        _context.context().config().changed(true);
    }
}
void SSAOPreRenderOperator::prepare(const Camera* camera, GFX::CommandBuffer& bufferInOut) {
    if (_enabled) {
        RenderStateBlock blueChannelOnly = RenderStateBlock::get(_context.get2DStateBlock());
        blueChannelOnly.setColourWrites(true, false, false, false);

        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = _context.get2DStateBlock();

        _ssaoGenerateConstants.set(_ID("projectionMatrix"), GFX::PushConstantType::MAT4, camera->projectionMatrix());
        _ssaoGenerateConstants.set(_ID("invProjectionMatrix"), GFX::PushConstantType::MAT4, GetInverse(camera->projectionMatrix()));

        if(genHalfRes()) {
            { // DownSample depth and normals
                GFX::BeginRenderPassCommand beginRenderPassCmd = {};
                beginRenderPassCmd._target = _halfDepthAndNormals._targetID;
                beginRenderPassCmd._name = "DO_SSAO_DOWNSAMPLE_NORMALS";
                EnqueueCommand(bufferInOut, beginRenderPassCmd);

                pipelineDescriptor._shaderProgramHandle = _ssaoDownSampleShader->getGUID();
                EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

                GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
                const auto& depthAtt = _parent.screenRT()._rt->getAttachment(RTAttachmentType::Depth, 0);
                descriptorSetCmd._set._textureData.setTexture(depthAtt.texture()->data(), depthAtt.samplerHash(), TextureUsage::DEPTH);

                const auto& screenAtt = _parent.screenRT()._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY));
                descriptorSetCmd._set._textureData.setTexture(screenAtt.texture()->data(), screenAtt.samplerHash(), TextureUsage::SCENE_NORMALS);
                EnqueueCommand(bufferInOut, descriptorSetCmd);

                EnqueueCommand(bufferInOut, _triangleDrawCmd);

                EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
            }
            { // Generate Half Res AO
                GFX::BeginRenderPassCommand beginRenderPassCmd = {};
                beginRenderPassCmd._target = _ssaoHalfResOutput._targetID;
                beginRenderPassCmd._name = "DO_SSAO_HALF_RES_CALC";
                EnqueueCommand(bufferInOut, beginRenderPassCmd);

                pipelineDescriptor._shaderProgramHandle = _ssaoGenerateHalfResShader->getGUID();
                EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

                EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _ssaoGenerateConstants });

                GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
                descriptorSetCmd._set._textureData.setTexture(_noiseTexture->data(), _noiseSampler, TextureUsage::UNIT0);

                const auto& depthAtt = _halfDepthAndNormals._rt->getAttachment(RTAttachmentType::Colour, 0);
                descriptorSetCmd._set._textureData.setTexture(depthAtt.texture()->data(), depthAtt.samplerHash(), TextureUsage::DEPTH);

                EnqueueCommand(bufferInOut, descriptorSetCmd);

                EnqueueCommand(bufferInOut, _triangleDrawCmd);

                EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
            }
            { // UpSample AO
                GFX::BeginRenderPassCommand beginRenderPassCmd = {};
                beginRenderPassCmd._target = _ssaoOutput._targetID;
                beginRenderPassCmd._name = "DO_SSAO_UPSAMPLE_AO";
                EnqueueCommand(bufferInOut, beginRenderPassCmd);

                pipelineDescriptor._shaderProgramHandle = _ssaoUpSampleShader->getGUID();
                EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

                SamplerDescriptor linearSampler = {};
                linearSampler.wrapUVW(TextureWrap::CLAMP_TO_EDGE);
                linearSampler.minFilter(TextureFilter::LINEAR);
                linearSampler.magFilter(TextureFilter::LINEAR);
                linearSampler.anisotropyLevel(0);

                GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
                descriptorSetCmd._set._textureData.setTexture(_noiseTexture->data(), _noiseSampler, TextureUsage::UNIT0);

                const auto& halfResAOAtt = _ssaoHalfResOutput._rt->getAttachment(RTAttachmentType::Colour, 0);
                descriptorSetCmd._set._textureData.setTexture(halfResAOAtt.texture()->data(), linearSampler.getHash(), TextureUsage::UNIT0);
                descriptorSetCmd._set._textureData.setTexture(halfResAOAtt.texture()->data(), halfResAOAtt.samplerHash(), TextureUsage::UNIT1);

                const auto& halfDepthAtt = _halfDepthAndNormals._rt->getAttachment(RTAttachmentType::Colour, 0);
                descriptorSetCmd._set._textureData.setTexture(halfDepthAtt.texture()->data(), halfDepthAtt.samplerHash(), TextureUsage::NORMALMAP);

                const auto& depthAtt = _parent.screenRT()._rt->getAttachment(RTAttachmentType::Depth, 0);
                descriptorSetCmd._set._textureData.setTexture(depthAtt.texture()->data(), depthAtt.samplerHash(), TextureUsage::DEPTH);

                EnqueueCommand(bufferInOut, descriptorSetCmd);

                EnqueueCommand(bufferInOut, _triangleDrawCmd);

                EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
            }
        } else {
            { // Generate Full Res AO
                GFX::BeginRenderPassCommand beginRenderPassCmd = {};
                beginRenderPassCmd._target = _ssaoOutput._targetID;
                beginRenderPassCmd._name = "DO_SSAO_CALC";
                EnqueueCommand(bufferInOut, beginRenderPassCmd);

                pipelineDescriptor._shaderProgramHandle = _ssaoGenerateShader->getGUID();
                EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

                GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
                descriptorSetCmd._set._textureData.setTexture(_noiseTexture->data(), _noiseSampler, TextureUsage::UNIT0);

                const auto& depthAtt = _parent.screenRT()._rt->getAttachment(RTAttachmentType::Depth, 0);
                descriptorSetCmd._set._textureData.setTexture(depthAtt.texture()->data(), depthAtt.samplerHash(), TextureUsage::DEPTH);

                const auto& screenAtt = _parent.screenRT()._rt->getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY));
                descriptorSetCmd._set._textureData.setTexture(screenAtt.texture()->data(), screenAtt.samplerHash(), TextureUsage::SCENE_NORMALS);
                EnqueueCommand(bufferInOut, descriptorSetCmd);

                EnqueueCommand(bufferInOut, GFX::SendPushConstantsCommand{ _ssaoGenerateConstants });

                EnqueueCommand(bufferInOut, _triangleDrawCmd);

                EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
            }
        }
        {
            // Blur AO
            GFX::BeginRenderPassCommand beginRenderPassCmd = {};
            beginRenderPassCmd._descriptor.drawMask().disableAll();
            beginRenderPassCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::EXTRA), true);
            beginRenderPassCmd._target = _parent.screenRT()._targetID;
            beginRenderPassCmd._name = "DO_SSAO_BLUR";
            EnqueueCommand(bufferInOut, beginRenderPassCmd);

            pipelineDescriptor._shaderProgramHandle = blurResults() ? _ssaoBlurShader->getGUID()
                                                                    : _ssaoPassThroughShader->getGUID();
                 
            pipelineDescriptor._stateHash = blueChannelOnly.getHash();
            EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

            const auto& ssaoAtt = _ssaoOutput._rt->getAttachment(RTAttachmentType::Colour, 0);

            GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
            descriptorSetCmd._set._textureData.setTexture(ssaoAtt.texture()->data(), ssaoAtt.samplerHash(),TextureUsage::UNIT0);
            EnqueueCommand(bufferInOut, descriptorSetCmd);

            EnqueueCommand(bufferInOut, _triangleDrawCmd);

            EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
        }
    }
}

bool SSAOPreRenderOperator::execute(const Camera* camera, const RenderTargetHandle& input, const RenderTargetHandle& output, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(camera);
    ACKNOWLEDGE_UNUSED(input);
    ACKNOWLEDGE_UNUSED(output);
    ACKNOWLEDGE_UNUSED(bufferInOut);

    return false;
}

void SSAOPreRenderOperator::onToggle(const bool state) {
    PreRenderOperator::onToggle(state);
    _enabled = state;
}
}