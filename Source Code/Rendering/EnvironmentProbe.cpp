#include "Headers/EnvironmentProbe.h"

#include "Dynamics/Entities/Headers/Impostor.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

namespace {
    U32 g_maxEnvironmentProbes = 32;
};

vectorImpl<bool> EnvironmentProbe::s_availableSlices;
RenderTargetHandle EnvironmentProbe::s_reflection;

EnvironmentProbe::EnvironmentProbe(ProbeType type) :
    GUIDWrapper(),
    _type(type),
    _updateRate(1),
    _currentUpdateCall(0),
    _aabb(vec3<F32>(-1), vec3<F32>(1))
{
    assert(!s_availableSlices.empty());
        
    _currentArrayIndex = allocateSlice();

    RenderStateBlock primitiveStateBlock;
    _boundingBoxPrimitive = GFX_DEVICE.getOrCreatePrimitive(false);
    _boundingBoxPrimitive->name(Util::StringFormat("EnvironmentProbe_%d", getGUID()));
    _boundingBoxPrimitive->stateHash(primitiveStateBlock.getHash());

    _impostor = CreateResource<ImpostorSphere>(ResourceDescriptor(Util::StringFormat("EnvironmentProbeImpostor_%d", getGUID())));
    _impostor->setRadius(1.0f);

    _impostorShader = CreateResource<ShaderProgram>(ResourceDescriptor("ImmediateModeEmulation.EnvironmentProbe"));
}

EnvironmentProbe::~EnvironmentProbe()
{
    s_availableSlices[_currentArrayIndex] = false;
    _boundingBoxPrimitive->_canZombify = true;
}

void EnvironmentProbe::initStaticData() {
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

    RenderTargetHandle tempHandle;
    s_reflection = GFX_DEVICE.allocateRT(RenderTargetID::ENVIRONMENT, false);
    s_reflection._rt->addAttachment(environmentDescriptor, RTAttachment::Type::Colour, 0);
    s_reflection._rt->useAutoDepthBuffer(true);
    s_reflection._rt->create(Config::REFLECTION_TARGET_RESOLUTION);
    s_reflection._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());
}

void EnvironmentProbe::destroyStaticData() {
    GFX_DEVICE.deallocateRT(s_reflection);
}

U32 EnvironmentProbe::allocateSlice() {
    U32 i = 0;
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
        GFX_DEVICE.generateCubeMap(*s_reflection._rt,
                                   _currentArrayIndex,
                                   _aabb.getCenter(),
                                   vec2<F32>(0.1f, (_aabb.getMax() - _aabb.getCenter()).length()),
                                   RenderStage::REFLECTION,
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

void EnvironmentProbe::debugDraw() {
    static RenderStateBlock stateBlock;

    _boundingBoxPrimitive->paused(false);
    s_reflection._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::REFLECTION),
                           RTAttachment::Type::Colour,
                           0);

    VertexBuffer* vb = _impostor->getGeometryVB();
    GenericDrawCommand cmd(PrimitiveType::TRIANGLE_STRIP, 0, vb->getIndexCount());
    cmd.stateHash(stateBlock.getHash());
    cmd.shaderProgram(_impostorShader);
    cmd.sourceBuffer(vb);
    GFX_DEVICE.draw(cmd);
}

void EnvironmentProbe::updateInternal() {
    _boundingBoxPrimitive->fromBox(_aabb.getMin(), _aabb.getMax(), vec4<U8>(255, 255, 255, 255));
    const vec3<F32>& bbPos = _aabb.getCenter();
    _impostorShader->Uniform("dvd_WorldMatrixOverride", mat4<F32>(bbPos.x, bbPos.y, bbPos.z));
    _impostorShader->Uniform("dvd_LayerIndex", _currentArrayIndex);
}

}; //namespace Divide