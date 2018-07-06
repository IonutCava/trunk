/*
   Copyright (c) 2013 DIVIDE-Studio
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

#include "core.h"
#include "Particle.h"
#include "Graphs/Headers/SceneNode.h"

/// Basic definitions used by the particle emitter
/// By default , every PE node is inert until you pass it a descriptor
struct ParticleEmitterDescriptor {
	U32 _emissionInterval;         ///< time in milliseconds between particle emission
	U32 _emissionIntervalVariance; ///< time in milliseconds used to vary emission (_emissionInterval + rand(-_emissionIntervalVariance, _emissionIntervalVariance))

	F32 _velocity;				   ///< particle velocity on emission
	F32 _velocityVariance;		   ///< velocity variance (_velocity + rand(-_velocityVariance, _velocityVariance))

	U32 _lifetime;                 ///< lifetime , in milliseconds of each particle
    U32 _lifetimeVariance;          ///< liftime variance (_lifetime + rand(-_lifetimeVariance, _lifetimeVariance))

	ParticleEmitterDescriptor();
};

class Impostor;

/// A Particle emitter scene node. Nothing smarter to say, sorry :"> - Ionut
class ParticleEmitter : public SceneNode {
public:
	ParticleEmitter();
   ~ParticleEmitter();

    /// toggle the particle emitter on or off
    inline void enableEmitter(bool state) {_enabled = state;}

    /// Set the callback, the position and the radius of the trigger
	void setDescriptor(const ParticleEmitterDescriptor& descriptor);

	/// preprocess particles here
	void tick();

    ///Dummy function from SceneNode;
	void onDraw(const RenderStage& currentStage);

	///SceneNode concrete implementations
	bool unload();
	void postLoad(SceneGraphNode* const sgn);

	///When the SceneGraph calls the particle emitter's render function, we draw the impostor if needed
	virtual void render(SceneGraphNode* const sgn);

	///SceneNode test
	bool isInView(const bool distanceCheck,const BoundingBox& boundingBox,const BoundingSphere& sphere){return _drawImpostor;}

private:
	/// create particles
	bool _enabled;
	/// draw the impostor?
	bool _drawImpostor;
	/// used for debug rendering / editor
	Impostor* _impostor;
	/// pointers to important scenegraph information
	SceneGraphNode *_particleEmitterSGN, *_impostorSGN;
};

#endif