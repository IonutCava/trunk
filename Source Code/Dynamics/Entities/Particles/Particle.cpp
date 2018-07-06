#include "Headers/Particle.h"

namespace Divide {

ParticleDescriptor::ParticleDescriptor()
{
    pos[0] = pos[1] = pos[2] = 0.0f;
    speed[0] = speed[1] = speed[2] = 0.0f;
    rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
}

ParticleDescriptor::~ParticleDescriptor()
{
}

};