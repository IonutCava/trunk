#include "Headers/ParticleEmitter.h"

#include "Core/Headers/Application.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Scenes/Headers/SceneState.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

ParticleEmitterDescriptor::ParticleEmitterDescriptor() {
   _particleCount = 1000;
   _spread = 1.5f;
   _emissionInterval = 400;
   _emissionIntervalVariance = 75;
   _velocity           = 10.0f;
   _velocityVariance   = 1.0f;
   _lifetime           = getSecToMs(5.0f);
   _lifetimeVariance   = 25.0f;
   _textureFileName    = "particle.DDS";
}

ParticleEmitter::ParticleEmitter() : SceneNode(TYPE_PARTICLE_EMITTER),
                                    _drawImpostor(false),
                                    _updateParticleEmitterBB(true),
                                    _lastUsedParticle(0),
                                    _particlesCurrentCount(0),
                                    _particleStateBlockHash(0),
                                    _enabled(false),
                                    _uploaded(false),
                                    _created(false),
                                    _impostorSGN(nullptr),
                                    _particleTexture(nullptr),
                                    _particleShader(nullptr),
                                    _particleDepthShader(nullptr)
{
    _drawCommand = GenericDrawCommand(TRIANGLE_STRIP, 0, 4, 1);
    _readOffset = 0;
    _writeOffset = 2;
}

ParticleEmitter::~ParticleEmitter()
{
    unload();
}

bool ParticleEmitter::initData(){
    // assert if double init!
    assert(_particleGPUBuffer != nullptr);

    _particleGPUBuffer = GFX_DEVICE.newGVD(/*true*/false);
    _particleGPUBuffer->Create(3);
    
    // Not using Quad3D to improve performance
    static F32 particleQuad[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
    };

    _particleGPUBuffer->SetBuffer(0, 12, sizeof(F32), 1, particleQuad, false, false);
    _particleGPUBuffer->getDrawAttribDescriptor(VERTEX_POSITION_LOCATION).set(0, 0, 3, false, 0, 0, FLOAT_32);

    //Generate a render state
    RenderStateBlockDescriptor particleStateDesc;
    particleStateDesc.setCullMode(CULL_MODE_NONE);
    particleStateDesc.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);
    _particleStateBlockHash = GFX_DEVICE.getOrCreateStateBlock(particleStateDesc);

    ResourceDescriptor particleShaderDescriptor("particles");
    _particleShader = CreateResource<ShaderProgram>(particleShaderDescriptor);
    _particleShader->UniformTexture("depthBuffer", 1);
    REGISTER_TRACKED_DEPENDENCY(_particleShader);
    ResourceDescriptor particleDepthShaderDescriptor("particles.Depth");
    _particleDepthShader = CreateResource<ShaderProgram>(particleDepthShaderDescriptor);
    REGISTER_TRACKED_DEPENDENCY(_particleDepthShader);
    _impostor = New Impostor(_name);
    _renderState.addToDrawExclusionMask(SHADOW_STAGE);
    return (_particleShader != nullptr);
}

bool ParticleEmitter::unload(){
    if(getState() != RES_LOADED && getState() != RES_LOADING)
        return true;

    UNREGISTER_TRACKED_DEPENDENCY(_particleTexture);
    UNREGISTER_TRACKED_DEPENDENCY(_particleShader);
    UNREGISTER_TRACKED_DEPENDENCY(_particleDepthShader);
    RemoveResource(_particleTexture);
    RemoveResource(_particleShader);
    RemoveResource(_particleDepthShader);
   
    SAFE_DELETE(_particleGPUBuffer);
    
    _particles.clear();
    _particlePositionData.clear();
    _particleColorData.clear();
    _created = false;
    return SceneNode::unload();
}

void ParticleEmitter::postLoad(SceneGraphNode* const sgn){
    ///Hold a pointer to the trigger's location in the SceneGraph
    _particleEmitterSGN = sgn;
    
    _impostor->getSceneNodeRenderState().setDrawState(false);
    _impostorSGN = _particleEmitterSGN->addNode(_impostor);
    _impostorSGN->setActive(false);
    _impostor->getMaterial()->setDiffuse(vec4<F32>(0.0f, 0.0f, 1.0f, 1.0f));
    _impostor->getMaterial()->setAmbient(vec4<F32>(0.0f, 0.0f, 1.0f, 1.0f));
    
    _updateParticleEmitterBB = true;

    setState(RES_LOADED);

    SceneNode::postLoad(sgn);
}

bool ParticleEmitter::computeBoundingBox(SceneGraphNode* const sgn){
    if(!_enabled || !_created)
        return false;

    _updateParticleEmitterBB = true;
    sgn->getBoundingBox().set(vec3<F32>(-5), vec3<F32>(5));
    return SceneNode::computeBoundingBox(sgn);
}

void ParticleEmitter::onCameraChange(SceneGraphNode* const sgn){
    const mat4<F32>& viewMatrixCache = GFX_DEVICE.getMatrix(VIEW_MATRIX);
    _particleShader->Uniform("CameraRight_worldspace",      vec3<F32>(viewMatrixCache.m[0][0], viewMatrixCache.m[1][0], viewMatrixCache.m[2][0]));
    _particleShader->Uniform("CameraUp_worldspace",         vec3<F32>(viewMatrixCache.m[0][1], viewMatrixCache.m[1][1], viewMatrixCache.m[2][1]));
    _particleDepthShader->Uniform("CameraRight_worldspace", vec3<F32>(viewMatrixCache.m[0][0], viewMatrixCache.m[1][0], viewMatrixCache.m[2][0]));
    _particleDepthShader->Uniform("CameraUp_worldspace",    vec3<F32>(viewMatrixCache.m[0][1], viewMatrixCache.m[1][1], viewMatrixCache.m[2][1]));
}

ShaderProgram* const ParticleEmitter::getDrawShader(RenderStage renderStage){
    return renderStage == FINAL_STAGE ? _particleShader : _particleDepthShader;
}

///When the SceneGraph calls the particle emitter's render function, we draw the impostor if needed
void ParticleEmitter::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    if(_particlesCurrentCount > 0 && _enabled && _created){
        _particleTexture->Bind(ShaderProgram::TEXTURE_UNIT0);
        GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Bind(1, TextureDescriptor::Depth);

        _drawCommand.renderWireframe(sgn->renderWireframe());
        _drawCommand.stateHash(_particleStateBlockHash);
        _drawCommand.instanceCount(_particlesCurrentCount);
        _drawCommand.drawID(GFX_DEVICE.getDrawID(sgn->getGUID()));
        _drawCommand.shaderProgram(getDrawShader(currentRenderStage));
        GFX_DEVICE.submitRenderCommand(_particleGPUBuffer, _drawCommand);
    }
}

///The descriptor defines the particle properties
void ParticleEmitter::setDescriptor(const ParticleEmitterDescriptor& descriptor){
    _descriptor = descriptor;
    _particleGPUBuffer->SetBuffer(1, descriptor._particleCount * 3, 4 * sizeof(F32), 1, NULL, true, true, /*true*/false);
    _particleGPUBuffer->SetBuffer(2, descriptor._particleCount * 3, 4 * sizeof(U8),  1, NULL, true, true, /*true*/false);

    _particleGPUBuffer->getDrawAttribDescriptor(13).set(1, 1, 4, false, 0, 0, FLOAT_32);
    _particleGPUBuffer->getDrawAttribDescriptor(VERTEX_COLOR_LOCATION).set(2, 1, 4, true,  0, 0, UNSIGNED_BYTE);

    _particles.resize(descriptor._particleCount);
    _particlePositionData.resize(descriptor._particleCount * 4);
    _particleColorData.resize(descriptor._particleCount * 4);
    for(U32 i = 0; i < descriptor._particleCount; ++i){
        _particles[i].life = -1.0f;
        _particles[i].distanceToCamera = -1.0f;
    }

    if(_particleTexture){
        UNREGISTER_TRACKED_DEPENDENCY(_particleTexture);
        RemoveResource(_particleTexture);
    }

    SamplerDescriptor textureSampler;
    ResourceDescriptor texture(descriptor._textureFileName);
    texture.setResourceLocation(ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/"+
                                ParamHandler::getInstance().getParam<std::string>("defaultTextureLocation")+"/"+
                                descriptor._textureFileName);
    texture.setFlag(true);
    texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);
    _particleTexture = CreateResource<Texture>(texture);
    REGISTER_TRACKED_DEPENDENCY(_particleTexture);
    _created = true;
}

void ParticleEmitter::uploadToGPU(){
    static const size_t attribSize_float = 4 * sizeof(F32);
    static const size_t attribSize_char  = 4 * sizeof(U8);
    if(_uploaded || !_created)
        return;

    U32 writeOffset = _writeOffset * (U32)_particles.size();
    U32 readOffset  = _readOffset  * (U32)_particles.size();

    _particleGPUBuffer->UpdateBuffer(1, _particlesCurrentCount, writeOffset, &_particlePositionData[0]);
    _particleGPUBuffer->UpdateBuffer(2, _particlesCurrentCount, writeOffset, &_particleColorData[0]);
        
    _particleGPUBuffer->getDrawAttribDescriptor(13).set(1, 1, 4, false, 0, readOffset, FLOAT_32);
    _particleGPUBuffer->getDrawAttribDescriptor(VERTEX_COLOR_LOCATION).set(2, 1, 4, true,  0, readOffset, UNSIGNED_BYTE);

    _writeOffset = (_writeOffset + 1) % 3;
    _readOffset  = (_readOffset + 1) % 3;

    _uploaded = true;
}

///The onDraw call will emit particles
bool ParticleEmitter::onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
    if(!_enabled || _particlesCurrentCount == 0 || !_created)
        return false;
    
    std::sort(_particles.begin(), _particles.end());
    uploadToGPU();

    return true;
}

/// Pre-process particles
void ParticleEmitter::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    if(!_enabled || !_created)
        return;

    if (_updateParticleEmitterBB) {
        sgn->updateBoundingBoxTransform(sgn->getWorldMatrix());
        _updateParticleEmitterBB = false;
    }

    F32 delta = getUsToSec(deltaTime);

    I32 newParticles = _descriptor._emissionInterval + random(-_descriptor._emissionIntervalVariance, _descriptor._emissionIntervalVariance);
    newParticles = (I32)(newParticles * delta) / (_lodLevel + 1);
    const Transform* transform = sgn->getComponent<PhysicsComponent>()->getConstTransform();
    const vec3<F32>& eyePos = sceneState.getRenderState().getCameraConst().getEye();
    const vec3<F32>& origin = transform->getPosition();
    const Quaternion<F32>& orientation = transform->getOrientation();
    F32 spread = _descriptor._spread;
    vec3<F32> mainDir = orientation * vec3<F32>(0, (_descriptor._velocity + random(-_descriptor._velocityVariance, _descriptor._velocityVariance)), 0);
    
    for(I32 i = 0; i < newParticles; ++i){
        ParticleDescriptor& currentParticle = _particles[findUnusedParticle()];
        currentParticle.life = _descriptor._lifetime + getMsToSec(random(-_descriptor._lifetimeVariance, _descriptor._lifetimeVariance));
        currentParticle.pos.set(origin);
        // Very bad way to generate a random direction;
        // See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
        // combined with some user-controlled parameters (main direction, spread, etc)
        currentParticle.speed  = mainDir + vec3<F32>((rand()%2000 - 1000.0f)/1000.0f,
                                                     (rand()%2000 - 1000.0f)/1000.0f,
                                                     (rand()%2000 - 1000.0f)/1000.0f) * spread;
         // Very bad way to generate a random color
        currentParticle.rgba.set(rand() % 256, rand() % 256, rand() % 256, (rand() % 256) / 3);
        currentParticle.size = (rand()%1000)/2000.0f + 0.1f;
    }

    // Simulate all particles
    I32 particlesCount = 0;
    vec3<F32> half_gravity = DEFAULT_GRAVITY * delta * 0.5f;
    for (ParticleDescriptor& p : _particles){
        if(p.life > 0.0f){
            // Decrease life
            p.life -= delta;
            if (p.life > 0.0f){
 
                // Simulate simple physics : gravity only, no collisions
                p.speed += half_gravity;
                p.pos += p.speed * delta;
                p.distanceToCamera = p.pos.distanceSquared(eyePos);
 
                _particlePositionData[4*particlesCount+0] = p.pos.x;
                _particlePositionData[4*particlesCount+1] = p.pos.y;
                _particlePositionData[4*particlesCount+2] = p.pos.z;
                _particlePositionData[4*particlesCount+3] = p.size;
 
                _particleColorData[4*particlesCount+0] = p.rgba.r;
                _particleColorData[4*particlesCount+1] = p.rgba.g;
                _particleColorData[4*particlesCount+2] = p.rgba.b;
                _particleColorData[4*particlesCount+3] = p.rgba.a;
 
            }else{
                // Particles that just died will be put at the end of the buffer in SortParticles();
                p.distanceToCamera = -1.0f;
            }
 
            particlesCount++;
        }
    }
    _particlesCurrentCount = particlesCount;
    _uploaded = false;

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

// Finds a Particle in ParticlesContainer which isn't used yet. (i.e. life < 0);
I32 ParticleEmitter::findUnusedParticle(){
 
    for(U32 i = _lastUsedParticle; i < _particles.size(); ++i){
        if (_particles[i].life < 0.0f){
            _lastUsedParticle = i;
            return i;
        }
    }
 
    for(I32 i = 0; i < _lastUsedParticle; ++i){
        if (_particles[i].life < 0.0f){
            _lastUsedParticle = i;
            return i;
        }
    }
 
    return 0; // All particles are taken, override the first one
}

};