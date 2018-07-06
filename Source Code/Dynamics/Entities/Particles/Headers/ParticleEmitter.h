/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _PARTICLE_EMITTER_H_
#define _PARTICLE_EMITTER_H_

#include "Particle.h"
#include "Graphs/Headers/SceneNode.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Core/Resources/Headers/ResourceDescriptor.h"

namespace Divide {

/// Basic definitions used by the particle emitter
/// By default , every PE node is inert until you pass it a descriptor
class ParticleEmitterDescriptor : public PropertyDescriptor{
public:
    /// maximum number of particles for this emitter
    U32 _particleCount;
    /// particles per second
    I32 _emissionInterval;         
    /// particles per second used to vary emission (_emissionInterval + rand(-_emissionIntervalVariance,_emissionIntervalVariance))
    I32 _emissionIntervalVariance; 

    F32 _spread;
    /// particle velocity on emission
    F32 _velocity;                   
    /// velocity variance (_velocity + rand(-_velocityVariance, _velocityVariance))
    F32 _velocityVariance;           
    /// lifetime, in milliseconds of each particle
    I32 _lifetime;                  
    /// liftime variance (_lifetime + rand(-_lifetimeVariance, _lifetimeVariance))
    I32 _lifetimeVariance;          
    stringImpl _textureFileName;

public:
    ParticleEmitterDescriptor();
    ///All of these are default values that should be safe for any kind of texture usage
    inline void setDefaultValues() {
        _particleCount = 1000;
        _spread = 1.5f;
        _emissionInterval = 400;
        _emissionIntervalVariance = 75;
        _velocity = 10.0f;
        _velocityVariance = 1.0f;
        _lifetime = Time::SecondsToMilliseconds( 5.0f );
        _lifetimeVariance = 25.0f;
        _textureFileName = "particle.DDS";
    }

    ParticleEmitterDescriptor* clone() const { 
        return MemoryManager_NEW ParticleEmitterDescriptor(*this);
    }
};

class Texture;
class GenericVertexData;
/// A Particle emitter scene node. Nothing smarter to say, sorry :"> - Ionut
class ParticleEmitter : public SceneNode {
public:
    ParticleEmitter();
   ~ParticleEmitter();

    /// Dummy function from SceneNode;
    bool onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage);

    /// toggle the particle emitter on or off
    inline void enableEmitter(bool state) {_enabled = state;}

    /// Set the callback, the position and the radius of the trigger
    void setDescriptor(const ParticleEmitterDescriptor& descriptor);
    
    inline void setDrawImpostor(const bool state) {_drawImpostor = state;}

    ///SceneNode concrete implementations
    bool initData();
    bool unload();

    bool computeBoundingBox(SceneGraphNode* const sgn);

    ///SceneNode test
    bool isInView(const SceneRenderState& sceneRenderState, SceneGraphNode* const sgn, const bool distanceCheck = false) {
        if(_enabled && _impostor)
            return _impostor->isInView(sceneRenderState, sgn, distanceCheck);

        return false;
    }

    const ParticleEmitterDescriptor& getDescriptor() const {return _descriptor;}

protected:
    void postLoad(SceneGraphNode* const sgn);

    /// preprocess particles here
    void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);
    
    ///When the SceneGraph calls the particle emitter's render function, we draw the impostor if needed
    virtual void render(SceneGraphNode* const sgn, 
                        const SceneRenderState& sceneRenderState,
                        const RenderStage& currentRenderStage);
    void getDrawCommands(SceneGraphNode* const sgn,
                         const RenderStage& currentRenderStage,
                         SceneRenderState& sceneRenderState,
                         vectorImpl<GenericDrawCommand>& drawCommandsOut);
    void onCameraChange(SceneGraphNode* const sgn);

private:
    I32 findUnusedParticle();
    void uploadToGPU();

private:
    vectorImpl<ParticleDescriptor > _particles;
    vectorImpl<F32 > _particlePositionData;
    vectorImpl<U8 >  _particleColorData;

    U32 _readOffset, _writeOffset;
    I32 _lastUsedParticle;
    U32 _particlesCurrentCount;
    /// create particles
    bool _enabled;
    bool _created;
    bool _uploaded;
    /// draw the impostor?
    bool _drawImpostor;
    bool _updateParticleEmitterBB;
    /// used for debug rendering / editor
    Impostor* _impostor;

    GenericVertexData* _particleGPUBuffer;

    size_t _particleStateBlockHash;

    ShaderProgram* _particleShader;
    ShaderProgram* _particleDepthShader;
    Texture* _particleTexture;

    ParticleEmitterDescriptor _descriptor;

    GenericDrawCommand        _drawCommand;
};

}; //namespace Divide

#endif