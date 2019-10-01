#include "stdafx.h"

#include "Headers/EnvironmentProbe.h"

#include "Scenes/Headers/Scene.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

namespace {
    U16 g_maxEnvironmentProbes = 32;
};

vector<bool> EnvironmentProbe::s_availableSlices;
RenderTargetHandle EnvironmentProbe::s_reflection;

EnvironmentProbe::EnvironmentProbe(Scene& parentScene, ProbeType type) :
    GUIDWrapper(),
    _context(parentScene.context().gfx()),
    _type(type),
    _updateRate(1),
    _currentUpdateCall(0),
    _aabb(vec3<F32>(-1), vec3<F32>(1))
{
    assert(!s_availableSlices.empty());
        
    _currentArrayIndex = allocateSlice();

    RenderStateBlock primitiveStateBlock;

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = primitiveStateBlock.getHash();
    pipelineDescriptor._shaderProgramHandle = ShaderProgram::defaultShader()->getGUID();

    _boundingBoxPrimitive = _context.newIMP();
    _boundingBoxPrimitive->name(Util::StringFormat("EnvironmentProbe_%d", getGUID()));
    _boundingBoxPrimitive->pipeline(*_context.newPipeline(pipelineDescriptor));

    _impostor = CreateResource<ImpostorSphere>(parentScene.resourceCache(), ResourceDescriptor(Util::StringFormat("EnvironmentProbeImpostor_%d", getGUID())));
    _impostor->setRadius(1.0f);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "ImmediateModeEmulation.glsl";
    vertModule._variant = "EnvironmentProbe";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "ImmediateModeEmulation.glsl";
    fragModule._variant = "EnvironmentProbe";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor shaderResDesc("ImmediateModeEmulation.EnvironmentProbe");
    shaderResDesc.propertyDescriptor(shaderDescriptor);

    _impostorShader = CreateResource<ShaderProgram>(parentScene.resourceCache(), shaderResDesc);
}

EnvironmentProbe::~EnvironmentProbe()
{
    s_availableSlices[_currentArrayIndex] = false;
    _boundingBoxPrimitive->reset();
}

void EnvironmentProbe::onStartup(GFXDevice& context) {
    s_availableSlices.resize(g_maxEnvironmentProbes, false);
    // Reflection Targets
    SamplerDescriptor reflectionSampler = {};
    reflectionSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.minFilter(TextureFilter::NEAREST);
    reflectionSampler.magFilter(TextureFilter::NEAREST);

    TextureDescriptor environmentDescriptor(TextureType::TEXTURE_CUBE_MAP, GFXImageFormat::RGB, GFXDataFormat::UNSIGNED_BYTE);
    environmentDescriptor.samplerDescriptor(reflectionSampler);
    environmentDescriptor.setLayerCount(g_maxEnvironmentProbes);


    TextureDescriptor depthDescriptor(TextureType::TEXTURE_CUBE_MAP, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::UNSIGNED_INT);

    depthDescriptor.samplerDescriptor(reflectionSampler);

    vector<RTAttachmentDescriptor> att = {
        { environmentDescriptor, RTAttachmentType::Colour, 0, DefaultColours::WHITE },
        { depthDescriptor, RTAttachmentType::Depth },
    };

    RenderTargetDescriptor desc = {};
    desc._name = "EnvironmentProbe";
    desc._resolution = vec2<U16>(Config::REFLECTION_TARGET_RESOLUTION_ENVIRONMENT_PROBE);
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    s_reflection = context.renderTargetPool().allocateRT(RenderTargetUsage::ENVIRONMENT, desc);
}

void EnvironmentProbe::onShutdown(GFXDevice& context)
{
    context.renderTargetPool().deallocateRT(s_reflection);
}

U16 EnvironmentProbe::allocateSlice() {
    U16 i = 0;
    for (; i < g_maxEnvironmentProbes; ++i) {
        if (!s_availableSlices[i]) {
            s_availableSlices[i] = true;
            break;
        }
    }

    return i;
}

void EnvironmentProbe::refresh(GFX::CommandBuffer& bufferInOut) {
    if (++_currentUpdateCall % _updateRate == 0) {
        _context.generateCubeMap(s_reflection._targetID,
                                 _currentArrayIndex,
                                 _aabb.getCenter(),
                                 vec2<F32>(0.1f, (_aabb.getMax() - _aabb.getCenter()).length()),
                                 RenderStagePass(RenderStage::REFLECTION, RenderPassType::MAIN_PASS),
                                 getRTIndex(),
                                 bufferInOut);
        _currentUpdateCall = 0;
    }
}

void EnvironmentProbe::setBounds(const vec3<F32>& min, const vec3<F32>& max) {
    _aabb.set(min, max);
    updateInternal();
}

void EnvironmentProbe::setBounds(const vec3<F32>& center, F32 radius) {
    _aabb.createFromSphere(center, radius);
    updateInternal();
}

void EnvironmentProbe::setUpdateRate(U8 rate) {
    _updateRate = rate;
}

RenderTargetHandle EnvironmentProbe::reflectionTarget() {
    return s_reflection;
}

F32 EnvironmentProbe::distanceSqTo(const vec3<F32>& pos) const {
    return _aabb.getCenter().distanceSquared(pos);
}

U32 EnvironmentProbe::getRTIndex() const {
    return _currentArrayIndex;
}

void EnvironmentProbe::debugDraw(GFX::CommandBuffer& bufferInOut) {

    const Texture_ptr& reflectTex = s_reflection._rt->getAttachment(RTAttachmentType::Colour, 0).texture();

    VertexBuffer* vb = _impostor->getGeometryVB();

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.getDefaultStateBlock(false);
    pipelineDescriptor._shaderProgramHandle = _impostorShader->getGUID();

    GFX::BindPipelineCommand bindPipelineCmd;
    bindPipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.setTexture(reflectTex->data(), to_U8(ShaderProgram::TextureUsage::REFLECTION_CUBE));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::SendPushConstantsCommand pushConstants;
    PushConstants& constants = pushConstants._constants;
    const vec3<F32>& bbPos = _aabb.getCenter();
    constants.set("dvd_WorldMatrixOverride", GFX::PushConstantType::MAT4, mat4<F32>(bbPos.x, bbPos.y, bbPos.z));
    constants.set("dvd_LayerIndex", GFX::PushConstantType::UINT, to_U32(_currentArrayIndex));
    GFX::EnqueueCommand(bufferInOut, pushConstants);

    GenericDrawCommand cmd = {};
    cmd._cmd.indexCount = vb->getIndexCount();
    cmd._sourceBuffer = vb->handle();

    GFX::DrawCommand drawCommand = { cmd };
    GFX::EnqueueCommand(bufferInOut, drawCommand);

    bufferInOut.add(_boundingBoxPrimitive->toCommandBuffer());
}

void EnvironmentProbe::updateInternal() {
    _boundingBoxPrimitive->fromBox(_aabb.getMin(), _aabb.getMax(), UColour4(255, 255, 255, 255));
}

}; //namespace Divide