#include "Headers/ParticleEmitter.h"

#include "Core/Headers/Application.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Scenes/Headers/SceneState.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

ParticleEmitter::ParticleEmitter()
    : SceneNode(SceneNodeType::TYPE_PARTICLE_EMITTER),
      _drawImpostor(false),
      _updateParticleEmitterBB(true),
      _particleStateBlockHash(0),
      _enabled(false),
      _uploaded(false),
      _particleTexture(nullptr),
      _particleShader(nullptr),
      _particleGPUBuffer(nullptr),
      _particleDepthShader(nullptr) {
    _drawCommand = GenericDrawCommand(PrimitiveType::TRIANGLE_STRIP, 0, 4, 1);
    _readOffset = 0;
    _writeOffset = 2;
}

ParticleEmitter::~ParticleEmitter() { unload(); }

bool ParticleEmitter::initData(std::shared_ptr<ParticleData> particleData) {
    // assert if double init!
    DIVIDE_ASSERT(_particleGPUBuffer == nullptr,
                  "ParticleEmitter::initData error: Double initData detected!");

    _particleGPUBuffer = GFX_DEVICE.newGVD(/*true*/ false);
    _particleGPUBuffer->Create(3);

    // Not using Quad3D to improve performance
    static F32 particleQuad[] = {
        -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f,  0.0f, 0.5f, 0.5f,  0.0f,
    };

    _particleGPUBuffer->SetBuffer(0, 12, sizeof(F32), 1, particleQuad, false,
                                  false);
    _particleGPUBuffer->getDrawAttribDescriptor(
                            to_uint(
                                AttribLocation::VERTEX_POSITION_LOCATION))
        .set(0, 0, 3, false, 0, 0, GFXDataFormat::FLOAT_32);

    updateData(particleData);

    // Generate a render state
    RenderStateBlockDescriptor particleStateDesc;
    particleStateDesc.setCullMode(CullMode::CULL_MODE_NONE);
    particleStateDesc.setBlend(true, BlendProperty::BLEND_PROPERTY_SRC_ALPHA,
                               BlendProperty::BLEND_PROPERTY_INV_SRC_ALPHA);
    _particleStateBlockHash =
        GFX_DEVICE.getOrCreateStateBlock(particleStateDesc);

    ResourceDescriptor particleShaderDescriptor("particles");
    _particleShader = CreateResource<ShaderProgram>(particleShaderDescriptor);
    _particleShader->Uniform("depthBuffer", ShaderProgram::TextureUsage::TEXTURE_UNIT1);
    REGISTER_TRACKED_DEPENDENCY(_particleShader);

    ResourceDescriptor particleDepthShaderDescriptor("particles.Depth");
    _particleDepthShader =
        CreateResource<ShaderProgram>(particleDepthShaderDescriptor);
    REGISTER_TRACKED_DEPENDENCY(_particleDepthShader);

    _impostor =
        CreateResource<Impostor>(ResourceDescriptor(_name + "_impostor"));
    _impostor->renderState().setDrawState(false);
    _impostor->getMaterialTpl()->setDiffuse(vec4<F32>(0.0f, 0.0f, 1.0f, 1.0f));
    _impostor->getMaterialTpl()->setAmbient(vec4<F32>(0.0f, 0.0f, 1.0f, 1.0f));

    //_renderState.addToDrawExclusionMask(RenderStage::SHADOW_STAGE);

    return (_particleShader != nullptr);
}

bool ParticleEmitter::updateData(std::shared_ptr<ParticleData> particleData) {
    DIVIDE_ASSERT(particleData.get() != nullptr,
                  "ParticleEmitter::updateData error: Invalid particle data!");

    _particles = particleData;

    U32 particleCount = _particles->totalCount();

    _particleGPUBuffer->SetBuffer(1, particleCount * 3, 4 * sizeof(F32), 1,
                                  NULL, true, true, /*true*/ false);
    _particleGPUBuffer->SetBuffer(2, particleCount * 3, 4 * sizeof(U8), 1, NULL,
                                  true, true, /*true*/ false);

    _particleGPUBuffer->getDrawAttribDescriptor(13)
        .set(1, 1, 4, false, 0, 0, GFXDataFormat::FLOAT_32);
    _particleGPUBuffer->getDrawAttribDescriptor(
                            to_uint(AttribLocation::VERTEX_COLOR_LOCATION))
        .set(2, 1, 4, true, 0, 0, GFXDataFormat::UNSIGNED_BYTE);

    for (U32 i = 0; i < particleCount; ++i) {
        // Distance to camera (squared)
        _particles->_misc[i].w = -1.0f;
        _particles->_alive[i] = false;
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
            ParamHandler::getInstance().getParam<stringImpl>("assetsLocation") +
            "/" +
            ParamHandler::getInstance().getParam<stringImpl>(
                "defaultTextureLocation") +
            "/" + _particles->_textureFileName);
        texture.setFlag(true);
        texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);

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
    sgn.addNode(_impostor).setActive(false);
    SceneNode::postLoad(sgn);
}

bool ParticleEmitter::computeBoundingBox(SceneGraphNode& sgn) {
    if (!_enabled) {
        return false;
    }
    DIVIDE_ASSERT(_particles.get() != nullptr,
                  "ParticleEmitter::computeBoundingBox error: BoundingBox "
                  "calculation requested without valid particle data "
                  "available!");
    _updateParticleEmitterBB = true;
    sgn.getBoundingBox().set(vec3<F32>(-5), vec3<F32>(5));
    return SceneNode::computeBoundingBox(sgn);
}

void ParticleEmitter::onCameraChange(SceneGraphNode& sgn) {
    const mat4<F32>& viewMatrixCache =
        GFX_DEVICE.getMatrix(MATRIX_MODE::VIEW_MATRIX);
    _particleShader->Uniform(
        "CameraRight_worldspace",
        vec3<F32>(viewMatrixCache.m[0][0], viewMatrixCache.m[1][0],
                  viewMatrixCache.m[2][0]));
    _particleShader->Uniform(
        "CameraUp_worldspace",
        vec3<F32>(viewMatrixCache.m[0][1], viewMatrixCache.m[1][1],
                  viewMatrixCache.m[2][1]));
    _particleDepthShader->Uniform(
        "CameraRight_worldspace",
        vec3<F32>(viewMatrixCache.m[0][0], viewMatrixCache.m[1][0],
                  viewMatrixCache.m[2][0]));
    _particleDepthShader->Uniform(
        "CameraUp_worldspace",
        vec3<F32>(viewMatrixCache.m[0][1], viewMatrixCache.m[1][1],
                  viewMatrixCache.m[2][1]));
}

void ParticleEmitter::getDrawCommands(
    SceneGraphNode& sgn, RenderStage currentRenderStage,
    SceneRenderState& sceneRenderState,
    vectorImpl<GenericDrawCommand>& drawCommandsOut) {
    U32 particleCount = getAliveParticleCount();
    if (!_enabled || particleCount == 0) {
        return;
    }

    _drawCommand.renderWireframe(
        sgn.getComponent<RenderingComponent>()->renderWireframe());
    _drawCommand.stateHash(_particleStateBlockHash);
    _drawCommand.instanceCount(particleCount);
    _drawCommand.shaderProgram(currentRenderStage == RenderStage::DISPLAY_STAGE
                                   ? _particleShader
                                   : _particleDepthShader);
    _drawCommand.sourceBuffer(_particleGPUBuffer);
    drawCommandsOut.push_back(_drawCommand);
}

void ParticleEmitter::uploadToGPU() {
    static const size_t attribSize_float = 4 * sizeof(F32);
    static const size_t attribSize_char = 4 * sizeof(U8);
    if (_uploaded || getAliveParticleCount() == 0) {
        return;
    }

    U32 writeOffset = _writeOffset * (U32)_particles->totalCount();
    U32 readOffset = _readOffset * (U32)_particles->totalCount();

    _particleGPUBuffer->SetIndexBuffer(_particles->getSortedIndices(), true,
                                       true);
    _particleGPUBuffer->UpdateBuffer(1, _particles->aliveCount(), writeOffset,
                                     _particles->_position.data());
    _particleGPUBuffer->UpdateBuffer(2, _particles->aliveCount(), writeOffset,
                                     _particles->_color.data());

    _particleGPUBuffer->getDrawAttribDescriptor(13)
        .set(1, 1, 4, false, 0, readOffset, GFXDataFormat::FLOAT_32);
    _particleGPUBuffer->getDrawAttribDescriptor(
                            to_uint(AttribLocation::VERTEX_COLOR_LOCATION))
        .set(2, 1, 4, true, 0, readOffset, GFXDataFormat::UNSIGNED_BYTE);

    _writeOffset = (_writeOffset + 1) % 3;
    _readOffset = (_readOffset + 1) % 3;

    _uploaded = true;
}

/// The onDraw call will emit particles
bool ParticleEmitter::onDraw(SceneGraphNode& sgn,
                             RenderStage currentStage) {
    if (!_enabled || getAliveParticleCount() == 0) {
        return false;
    }
    _particles->sort();
    uploadToGPU();


    U32 particleCount = getAliveParticleCount();

    /*if (particleCount > 0 && _enabled) {
        _particleTexture->Bind(
            to_uint(ShaderProgram::TextureUsage::TEXTURE_UNIT0));
        GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::RENDER_TARGET_DEPTH)
            ->Bind(to_uint(ShaderProgram::TextureUsage::TEXTURE_UNIT1),
                   TextureDescriptor::AttachmentType::Depth);
        GFX_DEVICE.submitRenderCommand(
            sgn.getComponent<RenderingComponent>()->getDrawCommands());
    }*/

    return true;
}

/// Pre-process particles
void ParticleEmitter::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                                  SceneState& sceneState) {
    if (!_enabled || getAliveParticleCount() == 0) {
        return;
    }

    PhysicsComponent* const transform = sgn.getComponent<PhysicsComponent>();
    const vec3<F32>& eyePos =
        sceneState.getRenderState().getCameraConst().getEye();

    if (_updateParticleEmitterBB) {
        sgn.updateBoundingBoxTransform(transform->getWorldMatrix());
        _updateParticleEmitterBB = false;
    }

    for (std::shared_ptr<ParticleSource>& source : _sources) {
        source->emit(deltaTime, _particles.get());
    }

    U32 count = _particles->totalCount();
    U8 lodLevel = sgn.getComponent<RenderingComponent>()->lodLevel();
    for (U32 i = 0; i < count; ++i) {
        _particles->_misc[i].w =
            _particles->_position[i].xyz().distanceSquared(eyePos);
        _particles->_acceleration[i].set(0.0f);
        _particles->lodLevel(lodLevel);
    }

    for (std::shared_ptr<ParticleUpdater>& up : _updaters) {
        up->update(deltaTime, _particles.get());
    }

    // const vec3<F32>& origin = transform->getPosition();
    // const Quaternion<F32>& orientation = transform->getOrientation();

    _uploaded = false;

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

U32 ParticleEmitter::getAliveParticleCount() const {
    if (!_particles.get()) {
        return 0;
    }
    return _particles->aliveCount();
}
};
