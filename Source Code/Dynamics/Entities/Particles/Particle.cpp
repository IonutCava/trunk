#include "Headers/Particle.h"

ParticleDescriptor::ParticleDescriptor(){
   _drag                = 0.0f;
   _wind                = 1.0f;
   _gravity             = 0.0f;
   _velocityFactor      = 0.0f;
   _cAcceleration       = 0.0f;
   _lifetime            = 1000;
   _lifetimeVariance    = 0;
   _spinSpeed           = 1.0;
   _spinSpeedMin        = 0.0;
   _spinSpeedMax        = 0.0;

   for(U8 i=0; i<NUM_PARTICLE_STATES; i++ ) {
      _color[i] = vec4( 1.0f, 1.0f, 1.0f, 1.0f );
      _size[i] = 1.0;
   }

   _texCoords[0] = vec2(0.0,0.0);
   _texCoords[1] = vec2(0.0,1.0); 
   _texCoords[2] = vec2(1.0,1.0);
   _texCoords[3] = vec2(1.0,0.0);
}

bool Particle::build(ParticleDescriptor* const descriptor, const vec3& inheritedVelocity){

   _velocity += inheritedVelocity * (descriptor->_velocityFactor);
   _acceleration = _acceleration * (descriptor->_cAcceleration);
   _totalLifetime = descriptor->_lifetime;
   /// add a random variance (lifetime variance should always be less than totalLifetime)
   _totalLifetime += random(-descriptor->_lifetimeVariance ,descriptor->_lifetimeVariance);
   _spinSpeed = descriptor->_spinSpeed * random(descriptor->_spinSpeedMin, descriptor->_spinSpeedMax );
}