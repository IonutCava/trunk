/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _PARTICLE_EMITTER_H_
#define _PARTICLE_EMITTER_H_

#include "ParticleSource.h"
#include "ParticleUpdater.h"
#include "Graphs/Headers/SceneNode.h"
#include "Dynamics/Entities/Headers/Impostor.h"

/// original source code:
/// https://github.com/fenbf/particles/blob/public/particlesCode
namespace Divide {

class Texture;
class GenericVertexData;
/// A Particle emitter scene node. Nothing smarter to say, sorry :"> - Ionut
class ParticleEmitter : public SceneNode {
   public:
    ParticleEmitter();
    ~ParticleEmitter();

    /// Dummy function from SceneNode;
    bool onDraw(SceneGraphNode& sgn, RenderStage currentStage);

    /// toggle the particle emitter on or off
    inline void enableEmitter(bool state) { _enabled = state; }

    inline void setDrawImpostor(const bool state) { _drawImpostor = state; }

    bool initData(std::shared_ptr<ParticleData> particleData);
    bool updateData(std::shared_ptr<ParticleData> particleData);

    /// SceneNode concrete implementations
    bool unload();

    bool computeBoundingBox(SceneGraphNode& sgn);

    /// SceneNode test
    bool isInView(const SceneRenderState& sceneRenderState,
                  SceneGraphNode& sgn, const bool distanceCheck = false) {
        if (_enabled && _impostor) {
            return _impostor->isInView(sceneRenderState, sgn, distanceCheck);
        }

        return false;
    }

    inline void addUpdater(std::shared_ptr<ParticleUpdater> updater) {
        _updaters.push_back(updater);
    }

    inline void addSource(std::shared_ptr<ParticleSource> source) {
        _sources.push_back(source);
    }

    U32 getAliveParticleCount() const;

   protected:
    void postLoad(SceneGraphNode& sgn);

    /// preprocess particles here
    void sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                     SceneState& sceneState);

    void getDrawCommands(SceneGraphNode& sgn,
                         RenderStage currentRenderStage,
                         SceneRenderState& sceneRenderState,
                         vectorImpl<GenericDrawCommand>& drawCommandsOut);
    void onCameraChange(SceneGraphNode& sgn);

   private:
    I32 findUnusedParticle();
    void uploadToGPU();

   private:
    std::shared_ptr<ParticleData> _particles;

    vectorImpl<std::shared_ptr<ParticleSource>> _sources;
    vectorImpl<std::shared_ptr<ParticleUpdater>> _updaters;

    U32 _readOffset, _writeOffset;
    /// create particles
    bool _enabled;
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
    GenericDrawCommand _drawCommand;
};

};  // namespace Divide

#endif