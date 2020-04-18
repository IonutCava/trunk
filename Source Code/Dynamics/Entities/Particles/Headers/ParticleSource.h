/*
   Copyright (c) 2018 DIVIDE-Studio
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

#ifndef _PARTICLE_SOURCE_H_
#define _PARTICLE_SOURCE_H_

#include "ParticleGenerator.h"

namespace Divide {

class ParticleSource {
   public:
    ParticleSource(GFXDevice& context);
    ParticleSource(GFXDevice& context, F32 emitRate);
    virtual ~ParticleSource();

    virtual void emit(const U64 deltaTimeUS, std::shared_ptr<ParticleData> p);

    inline void addGenerator(std::shared_ptr<ParticleGenerator> generator) {
        _particleGenerators.push_back(generator);
    }

    inline void updateEmitRate(F32 emitRate) { _emitRate = emitRate; }

    inline F32 emitRate() const { return _emitRate; }

    inline void updateTransform(const vec3<F32>& position, const Quaternion<F32>& orientation) {
        for (std::shared_ptr<ParticleGenerator> generator : _particleGenerators) {
            generator->updateTransform(position, orientation);
        }
    }

   protected:
    F32 _emitRate;
    GFXDevice& _context;
    vectorEASTL<std::shared_ptr<ParticleGenerator> > _particleGenerators;
};
};
#endif