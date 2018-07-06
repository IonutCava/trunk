/*
   Copyright (c) 2016 DIVIDE-Studio
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
    explicit ParticleEmitter(const stringImpl& name);
    ~ParticleEmitter();

    /// Dummy function from SceneNode;
    bool onRender(SceneGraphNode& sgn, RenderStage currentStage);

    /// toggle the particle emitter on or off
    inline void enableEmitter(bool state) { _enabled = state; }

    inline void setDrawImpostor(const bool state) { _drawImpostor = state; }

    bool initData(std::shared_ptr<ParticleData> particleData);
    bool updateData(std::shared_ptr<ParticleData> particleData);

    /// SceneNode concrete implementations
    bool unload();
    
    inline void addUpdater(std::shared_ptr<ParticleUpdater> updater) {
        _updaters.push_back(updater);
    }

    inline void addSource(std::shared_ptr<ParticleSource> source) {
        _sources.push_back(source);
    }

    U32 getAliveParticleCount() const;

   protected:
    void updateBoundsInternal(SceneGraphNode& sgn) override;

    void postLoad(SceneGraphNode& sgn)  override;

    /// preprocess particles here
    void sceneUpdate(const U64 deltaTime,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) override;

    bool getDrawCommands(SceneGraphNode& sgn,
                         RenderStage renderStage,
                         const SceneRenderState& sceneRenderState,
                         vectorImpl<GenericDrawCommand>& drawCommandsOut) override;
    void postUpdate();
   private:
    vec3<F32> _camUp, _camRight;
    std::shared_ptr<ParticleData> _particles;

    vectorImpl<std::shared_ptr<ParticleSource>> _sources;
    vectorImpl<std::shared_ptr<ParticleUpdater>> _updaters;

    /// create particles
    bool _enabled;
    /// draw the impostor?
    bool _drawImpostor;

    GenericVertexData* _particleGPUBuffer;

    size_t _particleStateBlockHash;
    size_t _particleStateBlockHashDepth;
    U64 _lastUpdateTimer;

    std::atomic_bool _updating;
    ShaderProgram* _particleShader;
    ShaderProgram* _particleDepthShader;
    Texture* _particleTexture;
};

};  // namespace Divide

#endif