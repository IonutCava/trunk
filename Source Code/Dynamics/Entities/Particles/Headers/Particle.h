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

#ifndef _PARTICLE_H_
#define _PARTICLE_H_

#include "core.h"
#include "Graphs/Headers/SceneNode.h"

/// Number of states per particle
#define NUM_PARTICLE_STATES  4
/// Descriptor used to build particles
class ParticleDescriptor {
public:
   vec4<F32> _color[ NUM_PARTICLE_STATES ];
   F32  _size[ NUM_PARTICLE_STATES ];
   F32  _time[ NUM_PARTICLE_STATES ];

   /// lifetime , in milliseconds of each particle
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
   vec2<F32>  _texCoords[4];

public:
	ParticleDescriptor();
   ~ParticleDescriptor();
};

/// The structure of each particle
struct Particle {
   vec3<F32> _position;
   vec3<F32> _velocity;
   vec3<F32> _acceleration;
   vec3<F32> _orientation;
   U32  _totalLifetime;
   U32  _currentLifetime;
   vec3<F32> _color;
   F32  _size;
   F32  _spinSpeed;
   Particle *  _next;

   bool build(ParticleDescriptor* const descriptor, const vec3<F32>& inheritedVelocity);
};

#endif