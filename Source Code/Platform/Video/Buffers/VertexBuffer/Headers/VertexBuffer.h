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

#pragma once
#ifndef _VERTEX_BUFFER_OBJECT_H
#define _VERTEX_BUFFER_OBJECT_H

#include "config.h"

#include "VertexDataInterface.h"
#include "Core/Headers/ByteBuffer.h"
#include "Platform/Video/Headers/RenderStagePass.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"

namespace Divide {

/// Vertex Buffer interface class to allow API-independent implementation of data
/// This class does NOT represent an API-level VB, such as: GL_ARRAY_BUFFER / D3DVERTEXBUFFER
/// It is only a "buffer" for "vertex info" abstract of implementation. (e.g.:
/// OGL uses a vertex array object for this)

class NOINITVTABLE VertexBuffer : public VertexDataInterface {
   public:
    struct Vertex {
        vec3<F32> _position;
        vec2<F32> _texcoord;
        F32       _normal;
        F32       _tangent;
        UColour4  _colour;
        P32       _weights;
        P32       _indices;
        
        inline bool operator==(const Vertex& other) const {
            return _position == other._position &&
                   _normal == other._normal &&
                   _tangent == other._tangent &&
                   _colour == other._colour &&
                   _texcoord == other._texcoord &&
                   _weights.i  == other._weights.i &&
                   _indices.i == other._indices.i;
        }

        inline bool operator!=(const Vertex& other) const {
            return _position != other._position ||
                   _normal != other._normal ||
                   _tangent != other._tangent ||
                   _colour != other._colour ||
                   _texcoord != other._texcoord ||
                   _weights.i != other._weights.i ||
                   _indices.i != other._indices.i;
        }

    };

    VertexBuffer(GFXDevice& context);
    virtual ~VertexBuffer();

    virtual bool create(bool staticDraw = true);

    virtual void draw(const GenericDrawCommand& command, U32 cmdBufferOffset) = 0;

    inline void useLargeIndices(bool state = true) {
        assert(_hardwareIndicesL.empty() && _hardwareIndicesS.empty() &&
               "VertexBuffer error: Index format type specified before "
               "buffer creation!");
        _format = state ? GFXDataFormat::UNSIGNED_INT
                        : GFXDataFormat::UNSIGNED_SHORT;
    }

    inline void setVertexCount(U32 size) { 
        _data.resize(size);
    }

    inline size_t getVertexCount() const {
        return _data.size();
    }

    inline const vector<Vertex>& getVertices() const {
        return _data;
    }

    inline void reserveIndexCount(U32 size) {
        usesLargeIndices() ? _hardwareIndicesL.reserve(size)
                           : _hardwareIndicesS.reserve(size);
    }

    inline void resizeVertexCount(U32 size, const Vertex& defaultValue = Vertex()) {
        _data.resize(size, defaultValue);
    }

    inline const vec3<F32>& getPosition(U32 index) const {
        return _data[index]._position;
    }

    inline const vec2<F32>& getTexCoord(U32 index) const {
        return _data[index]._texcoord;
    }

    inline F32 getNormal(U32 index) const {
        return _data[index]._normal;
    }

    inline F32 getNormal(U32 index, vec3<F32>& normalOut) const {
        F32 normal = getNormal(index);
        Util::UNPACK_VEC3(normal, normalOut.x, normalOut.y, normalOut.z);
        return normal;
    }

    inline F32 getTangent(U32 index) const {
        return _data[index]._tangent;
    }

    inline F32 getTangent(U32 index, vec3<F32>& tangentOut) const {
        F32 tangent = getTangent(index);
        Util::UNPACK_VEC3(tangent, tangentOut.x, tangentOut.y, tangentOut.z);
        return tangent;
    }

    inline P32 getBoneIndices(U32 index) const {
        return _data[index]._indices;
    }

    inline P32 getBoneWeightsPacked(U32 index) const {
        return _data[index]._weights;
    }

    inline FColour4 getBoneWeights(U32 index) const {
        const P32& weight = _data[index]._weights;
        return FColour4(CHAR_TO_FLOAT_SNORM(weight.b[0]),
                        CHAR_TO_FLOAT_SNORM(weight.b[1]),
                        CHAR_TO_FLOAT_SNORM(weight.b[2]),
                        CHAR_TO_FLOAT_SNORM(weight.b[3]));
    }

    virtual bool queueRefresh() = 0;

    inline bool usesLargeIndices() const { 
        return _format == GFXDataFormat::UNSIGNED_INT;
    }

    inline U32 getIndexCount() const {
        return to_U32(usesLargeIndices() ? _hardwareIndicesL.size()
                                          : _hardwareIndicesS.size());
    }

    inline U32 getIndex(U32 index) const {
        assert(index < getIndexCount());

        return usesLargeIndices() ? _hardwareIndicesL[index]
                                  : _hardwareIndicesS[index];
    }

    template<typename T>
    const vectorBest<T>& getIndices() const {
        static_assert(false, "VertexBuffer::getIndices error: Need valid index data type!");
    }

    template <typename T>
    inline void addIndex(T index) {
        if (usesLargeIndices()) {
            _hardwareIndicesL.push_back(to_U32(index));
        } else {
            _hardwareIndicesS.push_back(to_U16(index));
        }
    }

    template <typename T>
    inline void addIndices(const vectorBest<T>& indices, bool containsRestartIndex) {
        if (usesLargeIndices()) {
            std::transform(std::cbegin(indices),
                           std::cend(indices),
                           std::back_inserter(_hardwareIndicesL),
                           static_caster<T, U32>());
                           
        } else {
            std::transform(std::cbegin(indices),
                           std::cend(indices),
                           std::back_inserter(_hardwareIndicesS),
                           static_caster<T, U16>());
        }

        if (containsRestartIndex) {
            hasRestartIndex(true);
        }
    }

    inline void hasRestartIndex(bool state) {
        _primitiveRestartEnabled = state;
    }

    inline void addRestartIndex() {
        hasRestartIndex(true);
        if (usesLargeIndices()) {
            addIndex(Config::PRIMITIVE_RESTART_INDEX_L);
        } else {
            addIndex(Config::PRIMITIVE_RESTART_INDEX_S);
        }
    }

    inline void modifyPositionValues(U32 indexOffset, const vector<vec3<F32>>& newValues) {
       assert(indexOffset + newValues.size() - 1 < _data.size());
       DIVIDE_ASSERT(_staticBuffer == false ||
           (_staticBuffer == true && !_data.empty()),
           "VertexBuffer error: Modifying static buffers after creation is not allowed!");

       vector<Vertex>::iterator it = _data.begin() + indexOffset;
       for (const vec3<F32>& value : newValues) {
            (it++)->_position.set(value);
       }

       _attribDirty[to_base(AttribLocation::POSITION)] = true;
    }

    inline void modifyPositionValue(U32 index, const vec3<F32>& newValue) {
        modifyPositionValue(index, newValue.x, newValue.y, newValue.z);
    }

    inline void modifyPositionValue(U32 index, F32 x, F32 y, F32 z) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                      (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._position.set(x, y, z);
        _attribDirty[to_base(AttribLocation::POSITION)] = true;
    }

    inline void modifyColourValue(U32 index, const UColour4& newValue) {
        modifyColourValue(index, newValue.r, newValue.g, newValue.b, newValue.a);
    }

    inline void modifyColourValue(U32 index, U8 r, U8 g, U8 b, U8 a) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                      (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._colour.set(r, g, b, a);
        _attribDirty[to_base(AttribLocation::COLOR)] = true;
    }

    inline void modifyNormalValue(U32 index, const vec3<F32>& newValue) {
        modifyNormalValue(index, newValue.x, newValue.y, newValue.z);
    }

    inline void modifyNormalValue(U32 index, F32 x, F32 y, F32 z) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                      (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._normal = Util::PACK_VEC3(x, y, z);
        _attribDirty[to_base(AttribLocation::NORMAL)] = true;
    }

    inline void modifyTangentValue(U32 index, const vec3<F32>& newValue) {
        modifyTangentValue(index, newValue.x, newValue.y, newValue.z);
    }

    inline void modifyTangentValue(U32 index, F32 x, F32 y, F32 z) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                     (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._tangent = Util::PACK_VEC3(x, y, z);
        _attribDirty[to_base(AttribLocation::TANGENT)] = true;
    }

    inline void modifyTexCoordValue(U32 index, const vec2<F32>& newValue) {
        modifyTexCoordValue(index, newValue.s, newValue.t);
    }

    inline void modifyTexCoordValue(U32 index, F32 s, F32 t) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                      (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._texcoord.set(s, t);
        _attribDirty[to_base(AttribLocation::TEXCOORD)] = true;
    }

    inline void modifyBoneIndices(U32 index, P32 indices) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                      (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._indices = indices;
        _attribDirty[to_base(AttribLocation::BONE_INDICE)] = true;
    }

    inline void modifyBoneWeights(U32 index, const FColour4& weights) {
        P32 boneWeights;
        boneWeights.b[0] = FLOAT_TO_CHAR_SNORM(weights.x);
        boneWeights.b[1] = FLOAT_TO_CHAR_SNORM(weights.y);
        boneWeights.b[2] = FLOAT_TO_CHAR_SNORM(weights.z);
        boneWeights.b[3] = FLOAT_TO_CHAR_SNORM(weights.w);
        modifyBoneWeights(index, boneWeights);
    }

    inline void modifyBoneWeights(U32 index, P32 packedWeights) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                      (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._weights = packedWeights;
        _attribDirty[to_base(AttribLocation::BONE_WEIGHT)] = true;
    }

    inline size_t partitionBuffer() {
        U32 previousIndexCount = _partitions.empty() ? 0 : _partitions.back().second;
        U32 previousOffset = _partitions.empty() ? 0 : _partitions.back().first;
        U32 partitionedIndexCount = previousIndexCount + previousOffset;
        _partitions.push_back(std::make_pair(partitionedIndexCount, getIndexCount() - partitionedIndexCount));
        return _partitions.size() - 1;
    }

    inline U32 getPartitionIndexCount(U16 partitionID) {
        if (_partitions.empty()) {
            return getIndexCount();
        }
        assert(partitionID < _partitions.size() && "VertexBuffer error: Invalid partition offset!");
        return _partitions[partitionID].second;
    }

    inline U32 getPartitionOffset(U16 partitionID) {
        if (_partitions.empty()) {
            return 0;
        }
        assert(partitionID < _partitions.size() && "VertexBuffer error: Invalid partition offset!");
        return _partitions[partitionID].first;
    }

    virtual void reset() {
        _staticBuffer = false;
        _primitiveRestartEnabled = false;
        _partitions.clear();
        _data.clear();
        _hardwareIndicesL.clear();
        _hardwareIndicesS.clear();
        _attribDirty.fill(false);
    }

    void fromBuffer(VertexBuffer& other);
    bool deserialize(ByteBuffer& dataIn);
    bool serialize(ByteBuffer& dataOut) const;

    static void setAttribMasks(size_t count, const AttribFlags& flagMask);
    static void setAttribMask(size_t index, const AttribFlags& flagMask);

    inline void keepData(const bool state) {
        _keepDataInMemory = state;
    }

    virtual void computeNormals();
    virtual void computeTangents();

   protected:
    static vectorEASTL<AttribFlags> _attribMasks;

    virtual bool refresh() = 0;
    virtual bool createInternal();

    inline bool keepData() const {
        return _keepDataInMemory;
    }

   protected:
    /// Flag used to prevent clearing of the _data vector for static buffers
    bool _keepDataInMemory;
    /// If this flag is true, no further modification are allowed on the buffer (static geometry)
    bool _staticBuffer;
    /// The format of the buffer data
    GFXDataFormat _format;
    // first: offset, second: count
    vector<std::pair<U32, U32> > _partitions;
    /// Used for creating an "IB". If it's empty, then an outside source should
    /// provide the indices
    vectorBest<U32> _hardwareIndicesL;
    vectorBest<U16> _hardwareIndicesS;
    vector<Vertex> _data;
    /// Cache system to update only required data
    std::array<bool, to_base(AttribLocation::COUNT)> _attribDirty;
    bool _primitiveRestartEnabled;
};

template<>
inline const vectorBest<U32>& VertexBuffer::getIndices<U32>() const {
    return _hardwareIndicesL;
}

template<>
inline const vectorBest<U16>& VertexBuffer::getIndices<U16>() const {
    return _hardwareIndicesS;
}
};  // namespace Divide
#endif
