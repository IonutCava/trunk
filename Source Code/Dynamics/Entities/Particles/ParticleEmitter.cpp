#include "stdafx.h"

#include "config.h"

#include "Headers/ParticleEmitter.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/TaskPool.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Scenes/Headers/SceneState.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

namespace Divide {
namespace {
    // 3 should always be enough for round-robin GPU updates to avoid stalls:
    // 1 in ram, 1 in driver and 1 in VRAM
    static const U32 g_particleBufferSizeFactor = 3;
    static const U32 g_particleGeometryBuffer = 0;
    static const U32 g_particlePositionBuffer = g_particleGeometryBuffer + 1;
    static const U32 g_particleColourBuffer = g_particlePositionBuffer* + 2;

    static const U64 g_updateInterval = Time::MillisecondsToMicroseconds(33);
};

ParticleEmitter::ParticleEmitter(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : SceneNode(parentCache, descriptorHash, name, SceneNodeType::TYPE_PARTICLE_EMITTER),
      _context(context),
      _drawImpostor(false),
      _particleStateBlockHash(0),
      _particleStateBlockHashDepth(0),
      _enabled(false),
      _particleTexture(nullptr),
      _particleShader(nullptr),
      _particleDepthShader(nullptr)
{
    _tempBB.set(-VECTOR3_UNIT, VECTOR3_UNIT);
    for (U8 i = 0; i < s_MaxPlayerBuffers; ++i) {
        for (U8 j = 0; j < to_base(RenderStage::COUNT); ++j) {
            _particleGPUBuffers[i][j] = _context.newGVD(g_particleBufferSizeFactor);
        }
    }

    _buffersDirty.fill(false);
}

ParticleEmitter::~ParticleEmitter()
{ 
    unload(); 
}

GenericVertexData& ParticleEmitter::getDataBuffer(RenderStage stage, U8 playerIndex) {
    return *_particleGPUBuffers[playerIndex % s_MaxPlayerBuffers][to_U32(stage)];
}

bool ParticleEmitter::initData(const std::shared_ptr<ParticleData>& particleData) {
    // assert if double init!
    DIVIDE_ASSERT(particleData.get() != nullptr, "ParticleEmitter::updateData error: Invalid particle data!");
    _particles = particleData;
    const vectorImpl<vec3<F32>>& geometry = particleData->particleGeometryVertices();
    const vectorImpl<U32>& indices = particleData->particleGeometryIndices();

    for (U8 i = 0; i < s_MaxPlayerBuffers; ++i) {
        for (U8 j = 0; j < to_base(RenderStage::COUNT); ++j) {
            GenericVertexData& buffer = getDataBuffer(static_cast<RenderStage>(j), i);

            buffer.create(3);
            buffer.setBuffer(g_particleGeometryBuffer,
                             to_U32(geometry.size()),
                             sizeof(vec3<F32>),
                             false,
                             (bufferPtr)geometry.data(),
                             false,
                             false);
            if (!indices.empty()) {
                buffer.setIndexBuffer(to_U32(indices.size()), false, false, indices);
            }

            AttributeDescriptor& desc = buffer.attribDescriptor(to_base(AttribLocation::VERTEX_POSITION));
            desc.set(g_particleGeometryBuffer, 0, 3, false, 0, GFXDataFormat::FLOAT_32);
        }
    }

    updateData(particleData);

    // Generate a render state
    RenderStateBlock particleRenderState;
    particleRenderState.setCullMode(CullMode::NONE);
    _particleStateBlockHash = particleRenderState.getHash();

    particleRenderState.setZFunc(ComparisonFunction::LESS);
    _particleStateBlockHashDepth = particleRenderState.getHash();

    bool useTexture = _particleTexture != nullptr;
    ResourceDescriptor particleShaderDescriptor(useTexture ? "particles.WithTexture" : "particles.NoTexture");
    if (useTexture){
        particleShaderDescriptor.setPropertyList("HAS_TEXTURE");
    }
    _particleShader = CreateResource<ShaderProgram>(_parentCache, particleShaderDescriptor);

    ResourceDescriptor particleDepthShaderDescriptor("particles.Depth");
    _particleDepthShader = CreateResource<ShaderProgram>(_parentCache, particleDepthShaderDescriptor);

    if (_particleShader != nullptr) {
        Material_ptr mat = CreateResource<Material>(_parentCache, ResourceDescriptor("Material_particles"));

        mat->setShaderProgram(_particleShader);
        mat->setShaderProgram(_particleDepthShader, RenderStage::SHADOW);
        mat->setRenderStateBlock(_particleStateBlockHash, RenderPassType::COLOUR_PASS);
        mat->setRenderStateBlock(_particleStateBlockHashDepth, RenderPassType::DEPTH_PASS);
        setMaterialTpl(mat);

        return true;
    }

    return false;
}

bool ParticleEmitter::updateData(const std::shared_ptr<ParticleData>& particleData) {
    static const U32 positionAttribLocation = 13;
    static const U32 colourAttribLocation = to_base(AttribLocation::VERTEX_COLOR);

    U32 particleCount = _particles->totalCount();

    for (U8 i = 0; i < s_MaxPlayerBuffers; ++i) {
        for (U8 j = 0; j < to_base(RenderStage::COUNT); ++j) {
            GenericVertexData& buffer = getDataBuffer(static_cast<RenderStage>(j), i);

            buffer.setBuffer(g_particlePositionBuffer,
                             particleCount,
                             sizeof(vec4<F32>),
                             true,
                             NULL,
                             true,
                             true);
            buffer.setBuffer(g_particleColourBuffer,
                             particleCount,
                             sizeof(vec4<U8>),
                             true,
                             NULL,
                             true,
                             true);

            buffer.attribDescriptor(positionAttribLocation).set(g_particlePositionBuffer, 1, 4, false, 0, GFXDataFormat::FLOAT_32);

            buffer.attribDescriptor(colourAttribLocation).set(g_particleColourBuffer, 1, 4, true, 0, GFXDataFormat::UNSIGNED_BYTE);
        }
    }

    for (U32 i = 0; i < particleCount; ++i) {
        // Distance to camera (squared)
        _particles->_misc[i].w = -1.0f;
    }

    if (!_particles->_textureFileName.empty()) {
        SamplerDescriptor textureSampler;
        textureSampler.toggleSRGBColourSpace(true);

        TextureDescriptor textureDescriptor(TextureType::TEXTURE_2D);
        textureDescriptor.setSampler(textureSampler);

        ResourceDescriptor texture(_particles->_textureFileName);
        texture.setPropertyDescriptor(textureDescriptor);

        _particleTexture = CreateResource<Texture>(_parentCache, texture);
    }

    return true;
}

bool ParticleEmitter::unload() {
    if (getState() != ResourceState::RES_LOADED &&
        getState() != ResourceState::RES_LOADING) {
        return true;
    }
    
    _particles.reset();

    return SceneNode::unload();
}

void ParticleEmitter::postLoad(SceneGraphNode& sgn) {
    if (_particleTexture) {
        sgn.get<RenderingComponent>()->registerTextureDependency(_particleTexture->getData(),
                                                                 to_U8(ShaderProgram::TextureUsage::UNIT0));
    }

    setFlag(UpdateFlag::BOUNDS_CHANGED);

    sgn.get<BoundsComponent>()->lockBBTransforms(true);

    SceneNode::postLoad(sgn);
}

void ParticleEmitter::buildDrawCommands(SceneGraphNode& sgn,
                                        const RenderStagePass& renderStagePass,
                                        RenderPackage& pkgInOut) {

    U32 indexCount = to_U32(_particles->particleGeometryIndices().size());
    if (indexCount == 0) {
        indexCount = to_U32(_particles->particleGeometryVertices().size());
    }

    GenericDrawCommand cmd(_particles->particleGeometryType(), 0, indexCount);
    GFX::DrawCommand drawCommand;
    drawCommand._drawCommands.push_back(cmd);
    pkgInOut.addDrawCommand(drawCommand);

    const Pipeline& pipeline = pkgInOut.pipeline(0);
    PipelineDescriptor pipeDesc = pipeline.toDescriptor();

    pipeDesc._stateHash = (_context.isDepthStage(renderStagePass) ? _particleStateBlockHashDepth
                                                                  : _particleStateBlockHash); 
    pkgInOut.pipeline(0, _context.newPipeline(pipeDesc));
    

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

void ParticleEmitter::prepareForRender(const RenderStagePass& renderStagePass, const Camera& crtCamera) {
    if (renderStagePass.pass() != RenderPassType::DEPTH_PASS) {
        return;
    }

    const vec3<F32>& eyePos = crtCamera.getEye();
    U32 aliveCount = getAliveParticleCount();

    vectorImplBest<vec4<F32>>& misc = _particles->_misc;
    vectorImplBest<vec4<F32>>& pos = _particles->_position;

    auto updateDistToCamera = [&eyePos, &misc, &pos](const Task& parent, U32 start, U32 end) {
        for (U32 i = start; i < end; ++i) {
            misc[i].w = pos[i].xyz().distanceSquared(eyePos);
        }
    };

    parallel_for(_context.parent().platformContext(), updateDistToCamera, aliveCount, 1000);

    _bufferUpdate.emplace_back(CreateTask(_context.parent().platformContext(),
        [this, aliveCount, &renderStagePass](const Task& parentTask) {
        // invalidateCache means that the existing particle data is no longer partially sorted
        _particles->sort(true);
        _buffersDirty[to_U32(renderStagePass.stage())] = true;
    }));

    _bufferUpdate.back().startTask(Task::TaskPriority::HIGH);
}

/// The onRender call will emit particles
bool ParticleEmitter::onRender(SceneGraphNode& sgn,
                               const SceneRenderState& sceneRenderState,
                               const RenderStagePass& renderStagePass) {
    if ( _enabled &&  getAliveParticleCount() > 0) {
        for (TaskHandle& task : _bufferUpdate) {
            task.wait();
        }

        _bufferUpdate.clear();

        if (renderStagePass.pass() != RenderPassType::DEPTH_PASS && _buffersDirty[to_U32(renderStagePass.stage())]) {
            GenericVertexData& buffer = getDataBuffer(renderStagePass.stage(), sceneRenderState.playerPass());
            buffer.updateBuffer(g_particlePositionBuffer, to_U32(_particles->_renderingPositions.size()), 0, _particles->_renderingPositions.data());
            buffer.updateBuffer(g_particleColourBuffer, to_U32(_particles->_renderingColours.size()), 0, _particles->_renderingColours.data());
            buffer.incQueue();
            _buffersDirty[to_U32(renderStagePass.stage())] = false;
        }

        RenderingComponent* renderComp = sgn.get<RenderingComponent>();
        RenderPackage& pkg = renderComp->getDrawPackage(renderStagePass);

        GenericDrawCommand cmd = pkg.drawCommand(0, 0);
        cmd.cmd().primCount = to_U32(_particles->_renderingPositions.size());
        cmd.sourceBuffer(&getDataBuffer(renderStagePass.stage(), sceneRenderState.playerPass()));
        pkg.drawCommand(0, 0, cmd);

        prepareForRender(renderStagePass, *Camera::activeCamera());

        return SceneNode::onRender(sgn, sceneRenderState, renderStagePass);
    }

    return false;
}


/// Pre-process particles
void ParticleEmitter::sceneUpdate(const U64 deltaTime,
                                  SceneGraphNode& sgn,
                                  SceneState& sceneState) {

    static const U32 s_particlesPerThread = 1024;

    if (_enabled) {
        U32 aliveCount = getAliveParticleCount();
        renderState().setDrawState(aliveCount > 0);

        PhysicsComponent* transform = sgn.get<PhysicsComponent>();

        const vec3<F32>& pos = transform->getPosition();
        const Quaternion<F32>& rot = transform->getOrientation();

        F32 averageEmitRate = 0;
        for (std::shared_ptr<ParticleSource>& source : _sources) {
            source->updateTransform(pos, rot);
            source->emit(g_updateInterval, _particles);
            averageEmitRate += source->emitRate();
        }
        averageEmitRate /= _sources.size();

        aliveCount = getAliveParticleCount();

        auto updateSize = [this](const Task& parentTask, U32 start, U32 end) {
            for (U32 i = start; i < end; ++i) {
                _particles->_position[i].w = _particles->_misc[i].z;
                _particles->_acceleration[i].set(0.0f);
            }
        };

        parallel_for(_context.parent().platformContext(), updateSize, aliveCount, s_particlesPerThread, Task::TaskPriority::HIGH);

        ParticleData& data = *_particles;
        for (std::shared_ptr<ParticleUpdater>& up : _updaters) {
            up->update(g_updateInterval, data);
        }

        for (TaskHandle& task : _bbUpdate) {
            task.wait();
        }
        _bufferUpdate.clear();
        _bbUpdate.emplace_back(CreateTask(_context.parent().platformContext(), 
              [this, aliveCount, averageEmitRate](const Task& parentTask)
        {
            _tempBB.reset();
            for (U32 i = 0; i < aliveCount; i += to_U32(averageEmitRate) / 4) {
                _tempBB.add(_particles->_position[i]);
            }
            setFlag(UpdateFlag::BOUNDS_CHANGED);
        }));
        _bbUpdate.back().startTask(Task::TaskPriority::HIGH);
    }

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

U32 ParticleEmitter::getAliveParticleCount() const {
    if (!_particles.get()) {
        return 0;
    }
    return _particles->aliveCount();
}

void ParticleEmitter::updateBoundsInternal(SceneGraphNode& sgn) {
    for (TaskHandle& task : _bbUpdate) {
        task.wait();
    }
    _bufferUpdate.clear();

    U32 aliveCount = getAliveParticleCount();
    if (aliveCount > 2) {
        _boundingBox.set(_tempBB);
    } else {
        _boundingBox.set(-VECTOR3_UNIT, VECTOR3_UNIT);
    }

    SceneNode::updateBoundsInternal(sgn);
}

};
