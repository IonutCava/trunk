/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PARTICLE_EMITTER_H_
#define _PARTICLE_EMITTER_H_

#include "core.h"
#include "Particle.h"
#include "Graphs/Headers/SceneNode.h"

/// Basic definitions used by the particle emitter
/// By default , every PE node is inert until you pass it a descriptor
struct ParticleEmitterDescriptor {
	U32 _emissionInterval;         ///< time in miliseconds between particle emission
	U32 _emissionIntervalVariance; ///< time in miliseconds used to vary emission (_emissionInterval + rand(-_emissionIntervalVariance, _emissionIntervalVariance))

	F32 _velocity;				   ///< particle velocity on emission
	F32 _velocityVariance;		   ///< velocity variance (_velocity + rand(-_velocityVariance, _velocityVariance))

	U32 _lifetime;                 ///< lifetime , in miliseconds of each particle
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
	void onDraw();

	///SceneNode concrete implementations
	bool unload();
	void postLoad(SceneGraphNode* const sgn);	

	///When the SceneGraph calls the particle emitter's render function, we draw the impostor if needed
	virtual void render(SceneGraphNode* const sgn);

	///SceneNode test
	bool isInView(bool distanceCheck,BoundingBox& boundingBox){return _drawImpostor;}

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