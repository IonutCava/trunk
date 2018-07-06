/*
   Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _PARTICLE_DATA_H_
#define _PARTICLE_DATA_H_

#include "Graphs/Headers/SceneNode.h"

namespace Divide {

/// Container to store data for a given set of particles
class ParticleData {
   public:
    static const U32 g_threadPartitionSize = 256;

    enum class Properties : U32 {
        PROPERTIES_POS = toBit(1),
        PROPERTIES_VEL = toBit(2),
        PROPERTIES_ACC = toBit(3),
        PROPERTIES_COLOR = toBit(4),
        PROPERTIES_COLOR_TRANS = toBit(5),
        COUNT
    };
    /// helper array used for sorting
    vectorImplBest<std::pair<U32, F32>> _indices;
    vectorImplBest<vec4<F32>> _renderingPositions;
    vectorImplBest<vec4<U8>>  _renderingColours;
    /// x,y,z = position; w = size
    vectorImplBest<vec4<F32>> _position;
    /// x,y,z = _velocity; w = angle;
    vectorImplBest<vec4<F32>> _velocity;
    /// x,y,z = _acceleration; w = weight;
    vectorImplBest<vec4<F32>> _acceleration;
    /// x = time; y = interpolation; z = 1 / time;  w = distance to camera sq;
    vectorImplBest<vec4<F32>> _misc;
    /// r,g,b,a = colour and transparency
    vectorImplBest<vec4<F32>> _colour;
    /// r,g,b,a = colour and transparency
    vectorImplBest<vec4<F32>> _startColour;
    /// r,g,b,a = colour and transparency
    vectorImplBest<vec4<F32>> _endColour;
    /// Location of the texture file. Leave blank for colour only
    stringImpl _textureFileName;


    void setParticleGeometry(const vectorImpl<vec3<F32>>& particleGeometryVertices,
                             const vectorImpl<U32>& particleGeometryIndices,
                             PrimitiveType particleGeometryType);

    void setBillboarded(const bool state);

    inline PrimitiveType particleGeometryType() const {
        return _particleGeometryType;
    }

    inline const vectorImpl<vec3<F32>>& particleGeometryVertices() const {
        return _particleGeometryVertices;
    }

    inline const vectorImpl<U32>& particleGeometryIndices() const {
        return _particleGeometryIndices;
    }

    inline bool isBillboarded() const {
        return _isBillboarded;
    }
   public:
    explicit ParticleData(U32 particleCount, U32 optionsMask);
    ~ParticleData();

    void generateParticles(U32 particleCount, U32 optionsMask);
    void kill(U32 index);
    void wake(U32 index);
    void swapData(U32 indexA, U32 indexB);

    inline U32 aliveCount() const { return _aliveCount; }
    inline U32 totalCount() const { return _totalCount; }
    
    /// Sort ALIVE particles only
    void sort(bool invalidateCache);

   protected:
    U32 _totalCount;
    U32 _aliveCount;
    U32 _optionsMask;

    bool _isBillboarded;
    vectorImpl<vec3<F32>> _particleGeometryVertices;
    vectorImpl<U32> _particleGeometryIndices;
    PrimitiveType _particleGeometryType;
};

};  // namespace Divide

#endif