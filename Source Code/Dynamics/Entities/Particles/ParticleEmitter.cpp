#include "Headers/ParticleEmitter.h"

#include "Core/Headers/Application.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Scenes/Headers/SceneState.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/GenericVertexData.h"

namespace Divide {
namespace {
    static const U32 g_particleBufferSizeFactor = 5;
};

ParticleEmitter::ParticleEmitter()
    : SceneNode(SceneNodeType::TYPE_PARTICLE_EMITTER),
      _drawImpostor(false),
      _particleStateBlockHash(0),
      _particleStateBlockHashDepth(0),
      _enabled(false),
      _uploaded(false),
      _particleTexture(nullptr),
      _particleShader(nullptr),
      _particleGPUBuffer(nullptr),
      _particleDepthShader(nullptr)
{
}

ParticleEmitter::~ParticleEmitter() { 
    unload(); 
}

bool ParticleEmitter::initData(std::shared_ptr<ParticleData> particleData) {
    // assert if double init!
    DIVIDE_ASSERT(_particleGPUBuffer == nullptr,
                  "ParticleEmitter::initData error: Double initData detected!");

    _particleGPUBuffer = GFX_DEVICE.newGVD(true, g_particleBufferSizeFactor);
    _particleGPUBuffer->create(3);

    // Not using Quad3D to improve performance
    static F32 particleQuad[] = {
        -0.5f, -0.5f, 0.0f, 
         0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
    };

    _particleGPUBuffer->setBuffer(0, 12, sizeof(F32), false, particleQuad, false, false, true);
    _particleGPUBuffer->getDrawAttribDescriptor(to_const_uint(AttribLocation::VERTEX_POSITION))
        .set(0, 0, 3, false, 0, 0, GFXDataFormat::FLOAT_32);

    updateData(particleData);

    // Generate a render state
    RenderStateBlock particleRenderState;
    particleRenderState.setCullMode(CullMode::NONE);
    particleRenderState.setBlend(true,
                                 BlendProperty::SRC_ALPHA,
                                 BlendProperty::INV_SRC_ALPHA);
    particleRenderState.setZFunc(ComparisonFunction::LEQUAL);
    _particleStateBlockHash = particleRenderState.getHash();

    particleRenderState.setZFunc(ComparisonFunction::LESS);
    _particleStateBlockHashDepth = particleRenderState.getHash();

    bool useTexture = _particleTexture != nullptr;
    ResourceDescriptor particleShaderDescriptor(useTexture ? "particles.WithTexture" : "particles.NoTexture");
    if (useTexture){
        particleShaderDescriptor.setPropertyList("HAS_TEXTURE");
    }
    _particleShader = CreateResource<ShaderProgram>(particleShaderDescriptor);
    REGISTER_TRACKED_DEPENDENCY(_particleShader);

    ResourceDescriptor particleDepthShaderDescriptor("particles.Depth");
    _particleDepthShader = CreateResource<ShaderProgram>(particleDepthShaderDescriptor);
    REGISTER_TRACKED_DEPENDENCY(_particleDepthShader);

    //_renderState.addToDrawExclusionMask(RenderStage::SHADOW);

    return (_particleShader != nullptr);
}

bool ParticleEmitter::updateData(std::shared_ptr<ParticleData> particleData) {
    DIVIDE_ASSERT(particleData.get() != nullptr,
                  "ParticleEmitter::updateData error: Invalid particle data!");

    _particles = particleData;

    U32 particleCount = _particles->totalCount();

    _particleGPUBuffer->setBuffer(1,
                                  particleCount,
                                  4 * sizeof(F32),
                                  true,
                                  NULL, true, true, true);

    _particleGPUBuffer->setBuffer(2,
                                  particleCount,
                                  4 * sizeof(U8),
                                  true,
                                  NULL, true, true, true);

    _particleGPUBuffer->getDrawAttribDescriptor(13)
        .set(1, 1, 4, false, 4 * sizeof(F32), 0, GFXDataFormat::FLOAT_32);
    _particleGPUBuffer->getDrawAttribDescriptor(to_const_uint(AttribLocation::VERTEX_COLOR))
        .set(2, 1, 4, true, 4 * sizeof(U8), 0, GFXDataFormat::UNSIGNED_BYTE);

    for (U32 i = 0; i < particleCount; ++i) {
        // Distance to camera (squared)
        _particles->_misc[i].w = -1.0f;
    }

    if (_particleTexture) {
        UNREGISTER_TRACKED_DEPENDENCY(_particleTexture);
        RemoveResource(_particleTexture);
    }

    if (!_particles->_textureFileName.empty()) {
        SamplerDescriptor textureSampler;
        textureSampler.toggleSRGBColorSpace(true);

        ResourceDescriptor texture(_particles->_textureFileName);

        texture.setResourceLocation(
            ParamHandler::getInstance().getParam<stringImpl>("assetsLocation") + "/" +
            ParamHandler::getInstance().getParam<stringImpl>("defaultTextureLocation") + "/" +
            _particles->_textureFileName);

        texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);
        texture.setEnumValue(to_const_uint(TextureType::TEXTURE_2D));
        _particleTexture = CreateResource<Texture>(texture);

        REGISTER_TRACKED_DEPENDENCY(_particleTexture);
    }

    return true;
}

bool ParticleEmitter::unload() {
    if (getState() != ResourceState::RES_LOADED &&
        getState() != ResourceState::RES_LOADING) {
        return true;
    }
    if (_particleTexture) {
        UNREGISTER_TRACKED_DEPENDENCY(_particleTexture);
    }
    UNREGISTER_TRACKED_DEPENDENCY(_particleShader);
    UNREGISTER_TRACKED_DEPENDENCY(_particleDepthShader);
    RemoveResource(_particleTexture);
    RemoveResource(_particleShader);
    RemoveResource(_particleDepthShader);

    MemoryManager::DELETE(_particleGPUBuffer);

    _particles.reset();

    return SceneNode::unload();
}

void ParticleEmitter::postLoad(SceneGraphNode& sgn) {
    Framebuffer* depthBuffer = GFX_DEVICE.getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer;
    Texture* depthTexture = depthBuffer->getAttachment(TextureDescriptor::AttachmentType::Depth);
    TextureData depthBufferData = depthTexture->getData();
    depthBufferData.setHandleLow(to_const_uint(ShaderProgram::TextureUsage::DEPTH));
    sgn.getComponent<RenderingComponent>()->registerTextureDependency(depthBufferData);

    if (_particleTexture && _particleTexture->flushTextureState()) {
        TextureData particleTextureData = _particleTexture->getData();
        particleTextureData.setHandleLow(to_const_uint(ShaderProgram::TextureUsage::UNIT0));
        sgn.getComponent<RenderingComponent>()->registerTextureDependency(particleTextureData);
    }


    RenderingComponent* const renderable = sgn.getComponent<RenderingComponent>();
    assert(renderable != nullptr);

    GenericDrawCommand cmd(PrimitiveType::TRIANGLE_STRIP, 0, 4, 1);
    cmd.sourceBuffer(_particleGPUBuffer);
    for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
        GFXDevice::RenderPackage& pkg = 
            Attorney::RenderingCompSceneNode::getDrawPackage(*renderable, static_cast<RenderStage>(i));
            pkg._drawCommands.push_back(cmd);
    }
    sgn.lockBBTransforms(true);

    SceneNode::postLoad(sgn);
}

bool ParticleEmitter::getDrawCommands(SceneGraphNode& sgn,
                                      RenderStage renderStage,
                                      const SceneRenderState& sceneRenderState,
                                      vectorImpl<GenericDrawCommand>& drawCommandsOut) {

    U32 particleCount = getAliveParticleCount();
    if (!_enabled || particleCount == 0) {
        return false;
    }

    const Camera& camera = sceneRenderState.getCameraConst();

    vec3<F32> up(camera.getUpDir());
    vec3<F32> right(camera.getRightDir());

    if (_camUp != up) {
        _camUp.set(up);
        _particleShader->Uniform("CameraUp_worldspace", up);
        _particleDepthShader->Uniform("CameraUp_worldspace", up);
    }

    if (_camRight != right) {
        _camRight.set(right);
        _particleShader->Uniform("CameraRight_worldspace", right);
        _particleDepthShader->Uniform("CameraRight_worldspace", right);
    }

    GenericDrawCommand& cmd = drawCommandsOut.front();

    RenderingComponent* renderable = sgn.getComponent<RenderingComponent>();

    cmd.renderGeometry(renderable->renderGeometry());
    cmd.renderWireframe(renderable->renderWireframe());
    cmd.cmd().primCount = particleCount;
    cmd.stateHash(GFX_DEVICE.isDepthStage() ? _particleStateBlockHashDepth
                                            : _particleStateBlockHash);

    cmd.shaderProgram(renderStage != RenderStage::SHADOW
                                   ? _particleShader
                                   : _particleDepthShader);

    return SceneNode::getDrawCommands(sgn, renderStage, sceneRenderState, drawCommandsOut);
}

void ParticleEmitter::uploadToGPU() {
    if (_uploaded || getAliveParticleCount() == 0) {
        return;
    }
    
    _particleGPUBuffer->updateBuffer(1, _particles->aliveCount(), 0, _particles->_renderingPositions.data());
    _particleGPUBuffer->updateBuffer(2, _particles->aliveCount(), 0, _particles->_renderingColors.data());
    _particleGPUBuffer->incQueue();

    _uploaded = true;
}

/// The onDraw call will emit particles
bool ParticleEmitter::onDraw(SceneGraphNode& sgn,
                             RenderStage currentStage) {
    if (_enabled) {
        uploadToGPU();
        return true;
    }

    return false;
}

/// Pre-process particles
void ParticleEmitter::sceneUpdate(const U64 deltaTime,
                                  SceneGraphNode& sgn,
                                  SceneState& sceneState) {
    if (!_enabled && !_uploaded) {
        return;
    }

    bool validCount = getAliveParticleCount() > 0;
    renderState().setDrawState(validCount);

    _uploaded = false;

    PhysicsComponent* transform = sgn.getComponent<PhysicsComponent>();
    const vec3<F32>& eyePos = sceneState.renderState().getCameraConst().getEye();

    const vec3<F32>& pos = transform->getPosition();
    const Quaternion<F32>& rot = transform->getOrientation();

    F32 averageEmitRate = 0;
    for (std::shared_ptr<ParticleSource>& source : _sources) {
        source->updateTransform(pos, rot);
        source->emit(deltaTime, _particles);
        averageEmitRate += source->emitRate();
    }

    averageEmitRate /= _sources.size();

    U32 count = _particles->totalCount();
    for (U32 i = 0; i < count; ++i) {
        _particles->_misc[i].w =  _particles->_position[i].xyz().distanceSquared(eyePos);
        _particles->_position[i].w = 1.0f * _particles->_misc[i].z;
        _particles->_acceleration[i].set(0.0f);
    }

    for (std::shared_ptr<ParticleUpdater>& up : _updaters) {
        up->update(deltaTime, _particles);
    }

    // const vec3<F32>& origin = transform->getPosition();
    // const Quaternion<F32>& orientation = transform->getOrientation();

    // invalidateCache means that the existing particle data is no longer partially sorted
    _particles->sort(true);

    _boundingBox.first.reset();
    U32 aliveCount = _particles->aliveCount();
    for (U32 i = 0; i < aliveCount; i += to_uint(averageEmitRate) / 4) {
        _boundingBox.first.add(_particles->_position[i]);
    }
    _boundingBox.second = true;

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

U32 ParticleEmitter::getAliveParticleCount() const {
    if (!_particles.get()) {
        return 0;
    }
    return _particles->aliveCount();
}
};
