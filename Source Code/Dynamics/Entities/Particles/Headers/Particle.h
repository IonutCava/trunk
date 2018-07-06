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

namespace Divide {

/// Descriptor used to build particles
class ParticleDescriptor {
public:
    vec3<F32> pos, speed;
    vec4<U8> rgba;
    F32 size, angle, weight;
    F32 life; //< Remaining life of the particle. if < 0 : dead and unused.
    F32 distanceToCamera; //< Distance squared
    bool operator<(ParticleDescriptor& that){
        // Sort in reverse order : far particles drawn first.
        return this->distanceToCamera > that.distanceToCamera;
    }

public:
    ParticleDescriptor();
   ~ParticleDescriptor();
};

}; //namespace Divide

#endif