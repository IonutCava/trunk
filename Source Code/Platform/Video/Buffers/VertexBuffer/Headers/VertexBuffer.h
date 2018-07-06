/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _VERTEX_BUFFER_OBJECT_H
#define _VERTEX_BUFFER_OBJECT_H

#include "config.h"

#include "VertexDataInterface.h"
#include "Platform/Headers/ByteBuffer.h"

namespace Divide {

class ShaderProgram;
/// Vertex Buffer interface class to allow API-independent implementation of
/// data
/// This class does NOT represent an API-level VB, such as: GL_ARRAY_BUFFER /
/// D3DVERTEXBUFFER
/// It is only a "buffer" for "vertex info" abstract of implementation. (e.g.:
/// OGL uses a vertex array object for this)

class NOINITVTABLE VertexBuffer : public VertexDataInterface {
   public:
    enum class VertexAttribute : U32 {
        ATTRIB_POSITION = 0,
        ATTRIB_COLOR = 1,
        ATTRIB_NORMAL = 2,
        ATTRIB_TEXCOORD = 3,
        ATTRIB_TANGENT = 4,
        ATTRIB_BONE_WEIGHT = 5,
        ATTRIB_BONE_INDICE = 6,
        COUNT = 7
    };

    struct Vertex {
        vec3<F32> _position;
        F32       _normal;
        F32       _tangent;
        vec4<U8>  _colour;
        vec2<F32> _texcoord;
        P32       _weights;
        P32       _indices;
    };

    typedef std::array<bool, to_const_uint(VertexAttribute::COUNT)> AttribFlags;

    VertexBuffer(GFXDevice& context);
    virtual ~VertexBuffer();

    virtual bool create(bool staticDraw = true);

    virtual void destroy() = 0;

    virtual void draw(const GenericDrawCommand& command,
                      bool useCmdBuffer = false) = 0;

    inline void useLargeIndices(bool state = true) {
        assert(_hardwareIndicesL.empty() && _hardwareIndicesS.empty() &&
               "VertexBuffer error: Index format type specified before "
               "buffer creation!");
        _format = state ? GFXDataFormat::UNSIGNED_INT
                        : GFXDataFormat::UNSIGNED_SHORT;
    }

    inline void setVertexCount(U32 size) { _data.resize(size); }

    inline size_t getVertexCount() const {
        return _data.size();
    }

    inline const vectorImpl<Vertex>& getVertices() const {
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

    inline vec4<F32> getBoneWeights(U32 index) const {
        const P32& weight = _data[index]._weights;
        return vec4<F32>(CHAR_TO_FLOAT_SNORM(weight.b[0]),
                         CHAR_TO_FLOAT_SNORM(weight.b[1]),
                         CHAR_TO_FLOAT_SNORM(weight.b[2]),
                         CHAR_TO_FLOAT_SNORM(weight.b[3]));
    }

    virtual bool queueRefresh() = 0;

    inline bool usesLargeIndices() const { 
        return _format == GFXDataFormat::UNSIGNED_INT;
    }

    inline U32 getIndexCount() const {
        return to_uint(usesLargeIndices() ? _hardwareIndicesL.size()
                                          : _hardwareIndicesS.size());
    }

    inline U32 getIndex(U32 index) const {
        assert(index < getIndexCount());

        return usesLargeIndices() ? _hardwareIndicesL[index]
                                  : _hardwareIndicesS[index];
    }

    template<typename T>
    const vectorImplAligned<T>& getIndices() const {
        static_assert(false, "VertexBuffer::getIndices error: Need valid index data type!");
    }

    template <typename T>
    inline void addIndex(T index) {
        if (usesLargeIndices()) {
            _hardwareIndicesL.push_back(to_uint(index));
        } else {
            _hardwareIndicesS.push_back(to_ushort(index));
        }
    }

    inline void addRestartIndex() {
        _primitiveRestartEnabled = true;
        if (usesLargeIndices()) {
        	addIndex(Config::PRIMITIVE_RESTART_INDEX_L);
        } else {
            addIndex(Config::PRIMITIVE_RESTART_INDEX_S);
        }
    }

    inline void modifyPositionValues(U32 indexOffset, const vectorImpl<vec3<F32>>& newValues) {
       assert(indexOffset + newValues.size() - 1 < _data.size());
       DIVIDE_ASSERT(_staticBuffer == false ||
           (_staticBuffer == true && !_data.empty()),
           "VertexBuffer error: Modifying static buffers after creation is not allowed!");

       vectorImpl<Vertex>::iterator it = _data.begin() + indexOffset;
       for (const vec3<F32>& value : newValues) {
            (it++)->_position.set(value);
       }

       _attribDirty[to_const_uint(VertexAttribute::ATTRIB_POSITION)] = true;
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
        _attribDirty[to_const_uint(VertexAttribute::ATTRIB_POSITION)] = true;
    }

    inline void modifyColourValue(U32 index, const vec4<U8>& newValue) {
        modifyColourValue(index, newValue.r, newValue.g, newValue.b, newValue.a);
    }

    inline void modifyColourValue(U32 index, U8 r, U8 g, U8 b, U8 a) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                      (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._colour.set(r, g, b, a);
        _attribDirty[to_const_uint(VertexAttribute::ATTRIB_COLOR)] = true;
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
        _attribDirty[to_const_uint(VertexAttribute::ATTRIB_NORMAL)] = true;
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
        _attribDirty[to_const_uint(VertexAttribute::ATTRIB_TANGENT)] = true;
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
        _attribDirty[to_const_uint(VertexAttribute::ATTRIB_TEXCOORD)] = true;
    }

    inline void modifyBoneIndices(U32 index, P32 indices) {
        assert(index < _data.size());

        DIVIDE_ASSERT(_staticBuffer == false ||
                      (_staticBuffer == true && !_data.empty()),
                      "VertexBuffer error: Modifying static buffers after creation is not allowed!");

        _data[index]._indices = indices;
        _attribDirty[to_const_uint(VertexAttribute::ATTRIB_BONE_INDICE)] = true;
    }

    inline void modifyBoneWeights(U32 index, const vec4<F32>& weights) {
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
        _attribDirty[to_const_uint(VertexAttribute::ATTRIB_BONE_WEIGHT)] = true;
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

    static void setAttribMasks(const AttribFlags& flagMask) {
        for (AttribFlags& flags : _attribMaskPerStage) {
            flags = flagMask;
        }
    }

    static void setAttribMask(RenderStage stage, const AttribFlags& flagMask) {
        _attribMaskPerStage[to_uint(stage)] = flagMask;
    }

    inline void keepData(const bool state) {
        _keepDataInMemory = state;
    }

    virtual void computeNormals();
    virtual void computeTangents();

   protected:
    static std::array<AttribFlags, to_const_uint(RenderStage::COUNT)> _attribMaskPerStage;

    virtual void checkStatus() = 0;
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
    vectorImpl<std::pair<U32, U32> > _partitions;
    /// Used for creating an "IB". If it's empty, then an outside source should
    /// provide the indices
    vectorImplAligned<U32> _hardwareIndicesL;
    vectorImplAligned<U16> _hardwareIndicesS;
    vectorImpl<Vertex> _data;
    /// Cache system to update only required data
    std::array<bool, to_const_uint(VertexAttribute::COUNT)> _attribDirty;
    bool _primitiveRestartEnabled;
};

template<>
inline const vectorImplAligned<U32>& VertexBuffer::getIndices<U32>() const {
    return _hardwareIndicesL;
}

template<>
inline const vectorImplAligned<U16>& VertexBuffer::getIndices<U16>() const {
    return _hardwareIndicesS;
}
};  // namespace Divide
#endif
