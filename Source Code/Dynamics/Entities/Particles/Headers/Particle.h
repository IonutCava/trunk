/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _PARTICLE_H_
#define _PARTICLE_H_

#include "resource.h"
#include "Graphs/Headers/SceneNode.h"

/// Number of states per particle
#define NUM_PARTICLE_STATES  4
/// Descriptor used to build particles
class ParticleDescriptor {
public:
   vec4 _color[ NUM_PARTICLE_STATES ];
   F32  _size[ NUM_PARTICLE_STATES ];
   F32  _time[ NUM_PARTICLE_STATES ];
   
   /// lifetime , in miliseconds of each particle
   U32 _lifetime;     
   /// liftime variance (_lifetime + rand(-_lifetimeVariance, _lifetimeVariance))
   I32 _lifetimeVariance;        
	
   /// Physics coefficients
   F32   _drag;
   F32   _wind;
   F32   _gravity;

   F32   _velocityFactor;
   F32   _cAcceleration;


   F32 _spinSpeed;
   F32 _spinSpeedMin;
   F32 _spinSpeedMax;
   ///Default quad texture coordinates
   vec2  _texCoords[4];

public:
	ParticleDescriptor();
   ~ParticleDescriptor();
};


/// The structure of each particle
struct Particle {
   vec3 _position;  
   vec3 _velocity;
   vec3 _acceleration;
   vec3 _orientation;
   U32  _totalLifetime;
   U32  _currentLifetime;
   vec3 _color;
   F32  _size;
   F32  _spinSpeed;
   Particle *  _next;

   bool build(ParticleDescriptor* const descriptor, const vec3& inheritedVelocity);
};


#endif