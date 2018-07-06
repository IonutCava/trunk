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

vectorImpl<bool> EnvironmentProbe::s_availableSlices;
RenderTargetHandle EnvironmentProbe::s_reflection;

EnvironmentProbe::EnvironmentProbe(Scene& parentScene, ProbeType type) :
    GUIDWrapper(),
    _context(parentScene.platformContext().gfx()),
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

    _boundingBoxPrimitive = _context.newIMP();
    _boundingBoxPrimitive->name(Util::StringFormat("EnvironmentProbe_%d", getGUID()));
    _boundingBoxPrimitive->pipeline(_context.newPipeline(pipelineDescriptor));

    _impostor = CreateResource<ImpostorSphere>(parentScene.resourceCache(),
                                               ResourceDescriptor(Util::StringFormat("EnvironmentProbeImpostor_%d", getGUID())));
    _impostor->setRadius(1.0f);

    _impostorShader = CreateResource<ShaderProgram>(parentScene.resourceCache(), 
                                                    ResourceDescriptor("ImmediateModeEmulation.EnvironmentProbe"));
}

EnvironmentProbe::~EnvironmentProbe()
{
    s_availableSlices[_currentArrayIndex] = false;
    _boundingBoxPrimitive->clear();
}

void EnvironmentProbe::onStartup(GFXDevice& context) {
    s_availableSlices.resize(g_maxEnvironmentProbes, false);
    // Reflection Targets
    SamplerDescriptor reflectionSampler;
    reflectionSampler.setFilters(TextureFilter::NEAREST);
    reflectionSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.toggleMipMaps(false);

    TextureDescriptor environmentDescriptor(TextureType::TEXTURE_CUBE_MAP,
                                            GFXImageFormat::RGB8,
                                            GFXDataFormat::UNSIGNED_BYTE);
    environmentDescriptor.setSampler(reflectionSampler);
    environmentDescriptor.setLayerCount(g_maxEnvironmentProbes);


    TextureDescriptor depthDescriptor(TextureType::TEXTURE_CUBE_MAP,
                                      GFXImageFormat::DEPTH_COMPONENT,
                                      GFXDataFormat::UNSIGNED_INT);

    depthDescriptor.setSampler(reflectionSampler);

    RenderTargetHandle tempHandle;
    s_reflection = context.allocateRT(RenderTargetUsage::ENVIRONMENT, "EnviromentProbe");
    s_reflection._rt->addAttachment(environmentDescriptor, RTAttachment::Type::Colour, 0);
    s_reflection._rt->addAttachment(depthDescriptor, RTAttachment::Type::Depth, 0);
    s_reflection._rt->create(Config::REFLECTION_TARGET_RESOLUTION_ENVIRONMENT_PROBE, Config::REFLECTION_TARGET_RESOLUTION_ENVIRONMENT_PROBE);
    s_reflection._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());
}

void EnvironmentProbe::onShutdown(GFXDevice& context)
{
    context.deallocateRT(s_reflection);
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

void EnvironmentProbe::refresh() {
    if (++_currentUpdateCall % _updateRate == 0) {
        _context.generateCubeMap(s_reflection._targetID,
                                 _currentArrayIndex,
                                 _aabb.getCenter(),
                                 vec2<F32>(0.1f, (_aabb.getMax() - _aabb.getCenter()).length()),
                                 RenderStagePass(RenderStage::REFLECTION, RenderPassType::COLOUR_PASS),
                                 getRTIndex());
        _currentUpdateCall = 0;
    }
    _boundingBoxPrimitive->paused(true);
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

void EnvironmentProbe::debugDraw(RenderSubPassCmds& subPassesInOut) {
    _boundingBoxPrimitive->paused(false);

    const Texture_ptr& reflectTex = s_reflection._rt->getAttachment(RTAttachment::Type::Colour, 0).asTexture();

    VertexBuffer* vb = _impostor->getGeometryVB();

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.getDefaultStateBlock(false);
    pipelineDescriptor._shaderProgram = _impostorShader;

    GenericDrawCommand cmd(PrimitiveType::TRIANGLE_STRIP, 0, vb->getIndexCount());
    cmd.pipeline(_context.newPipeline(pipelineDescriptor));
    cmd.sourceBuffer(vb);

    RenderSubPassCmd newSubPass;
    newSubPass._textures.addTexture(TextureData(reflectTex->getTextureType(),
                                                reflectTex->getHandle(),
                                                to_U8(ShaderProgram::TextureUsage::REFLECTION_CUBE)));
    newSubPass._commands.push_back(cmd);
    newSubPass._commands.push_back(_boundingBoxPrimitive->toDrawCommand());
    subPassesInOut.push_back(newSubPass);
}

void EnvironmentProbe::updateInternal() {
    _boundingBoxPrimitive->fromBox(_aabb.getMin(), _aabb.getMax(), vec4<U8>(255, 255, 255, 255));
    const vec3<F32>& bbPos = _aabb.getCenter();
    _impostorShader->Uniform("dvd_WorldMatrixOverride", mat4<F32>(bbPos.x, bbPos.y, bbPos.z));
    _impostorShader->Uniform("dvd_LayerIndex", to_U32(_currentArrayIndex));
}

}; //namespace Divide