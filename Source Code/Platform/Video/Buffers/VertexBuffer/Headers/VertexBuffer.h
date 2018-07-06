/*
   Copyright (c) 2015 DIVIDE-Studio
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

#include "VertexDataInterface.h"

namespace Divide {

class ShaderProgram;
/// Vertex Buffer interface class to allow API-independent implementation of
/// data
/// This class does NOT represent an API-level VB, such as: GL_ARRAY_BUFFER /
/// D3DVERTEXBUFFER
/// It is only a "buffer" for "vertex info" abstract of implementation. (e.g.:
/// OGL uses a vertex array object for this)
class NOINITVTABLE VertexBuffer : public VertexDataInterface {
   protected:
    enum class VertexAttribute : U32 {
        ATTRIB_POSITION = 0,
        ATTRIB_COLOR = 1,
        ATTRIB_NORMAL = 2,
        ATTRIB_TEXCOORD = 3,
        ATTRIB_TANGENT = 4,
        ATTRIB_BONE_WEIGHT = 5,
        ATTRIB_BONE_INDICE = 6,
        COUNT
    };

   public:
    VertexBuffer()
        : VertexDataInterface(),
          _LODcount(0),
          _format(GFXDataFormat::UNSIGNED_SHORT),
          _indexDelimiter(0),
          _primitiveRestartEnabled(false),
          _created(false),
          _currentPartitionIndex(0),
          _largeIndices(false)
    {
        reset();
    }

    virtual ~VertexBuffer()
    {
        _LODcount = 1;
        reset();
    }

    virtual bool create(bool staticDraw = true) = 0;
    virtual void destroy() = 0;
    /// Some engine elements, like physics or some geometry shading techniques
    /// require a triangle list.
    virtual bool setActive() = 0;

    virtual void draw(const GenericDrawCommand& command,
                      bool useCmdBuffer = false) = 0;

    inline void setLODCount(const U8 LODcount) { _LODcount = LODcount; }

    inline void useLargeIndices(bool state = true) {
        DIVIDE_ASSERT(!_created,
                      "VertexBuffer error: Index format type specified before "
                      "buffer creation!");
        _largeIndices = state;
        _format = state ? GFXDataFormat::UNSIGNED_INT
                        : GFXDataFormat::UNSIGNED_SHORT;
    }

    inline void reservePositionCount(U32 size) { _dataPosition.reserve(size); }
    inline void reserveColourCount(U32 size) { _dataColor.reserve(size); }
    inline void reserveNormalCount(U32 size) { _dataNormal.reserve(size); }
    inline void reserveTangentCount(U32 size) { _dataTangent.reserve(size); }

    inline void reserveIndexCount(U32 size) {
        usesLargeIndices() ? _hardwareIndicesL.reserve(size)
                           : _hardwareIndicesS.reserve(size);
    }

    inline void resizePositionCount(
        U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO) {
        _dataPosition.resize(size, defaultValue);
    }

    inline void resizeColoCount(U32 size,
                                const vec4<U8>& defaultValue = vec4<U8>(255)) {
        _dataColor.resize(size, defaultValue);
    }

    inline void resizeNormalCount(
        U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO) {
        _dataNormal.resize(size, defaultValue);
    }

    inline void resizeTangentCount(
        U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO) {
        _dataTangent.resize(size, defaultValue);
    }

    inline vectorImpl<vec2<F32> >& getTexcoord() {
        _attribDirty[to_uint(VertexAttribute::ATTRIB_TEXCOORD)] = true;
        return _dataTexcoord;
    }
    inline vectorImpl<P32 >& getBoneIndices() {
        _attribDirty[to_uint(VertexAttribute::ATTRIB_BONE_INDICE)] = true;
        return _boneIndices;
    }
    inline vectorImpl<vec4<F32> >& getBoneWeights() {
        _attribDirty[to_uint(VertexAttribute::ATTRIB_BONE_WEIGHT)] = true;
        return _boneWeights;
    }

    inline const vectorImpl<vec3<F32> >& getPosition() const {
        return _dataPosition;
    }
    inline const vectorImpl<vec4<U8> >& getColor() const { return _dataColor; }
    inline const vectorImpl<vec3<F32> >& getNormal() const {
        return _dataNormal;
    }
    inline const vectorImpl<vec3<F32> >& getTangent() const {
        return _dataTangent;
    }
    inline const vec3<F32>& getPosition(U32 index) const {
        return _dataPosition[index];
    }
    inline const vec3<F32>& getNormal(U32 index) const {
        return _dataNormal[index];
    }
    inline const vec3<F32>& getTangent(U32 index) const {
        return _dataTangent[index];
    }

    virtual bool queueRefresh() = 0;

    inline bool usesLargeIndices() const { return _largeIndices; }

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
    const vectorImpl<T>& getIndices() const;/* {
        static_assert(false,
                "VertexBuffer::getIndices error: Need valid index data type!");
    }*/

    template <typename T>
    inline void addIndex(T index) {
        if (usesLargeIndices()) {
            _hardwareIndicesL.push_back(static_cast<U32>(index));
        } else {
            _hardwareIndicesS.push_back(static_cast<U16>(index));
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

    inline void addPosition(const vec3<F32>& pos) {
        _dataPosition.push_back(pos);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_POSITION)] = true;
    }

    inline void addColor(const vec4<U8>& col) {
        _dataColor.push_back(col);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_COLOR)] = true;
    }

    inline void addTexCoord(const vec2<F32>& texCoord) {
        _dataTexcoord.push_back(texCoord);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_TEXCOORD)] = true;
    }

    inline void addNormal(const vec3<F32>& norm) {
        _dataNormal.push_back(norm);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_NORMAL)] = true;
    }

    inline void addTangent(const vec3<F32>& tangent) {
        _dataTangent.push_back(tangent);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_TANGENT)] = true;
    }

    inline void addBoneIndex(const P32& idx) {
        _boneIndices.push_back(idx);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_BONE_INDICE)] = true;
    }

    inline void addBoneWeight(const vec4<F32>& idx) {
        _boneWeights.push_back(idx);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_BONE_WEIGHT)] = true;
    }

    inline void modifyPositionValue(U32 index, const vec3<F32>& newValue) {
        /*DIVIDE_ASSERT(index < _dataPosition.size(),
                      "VertexBuffer error: Invalid position offset!");*/
        _dataPosition[index].set(newValue);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_POSITION)] = true;
    }

    inline void modifyColorValue(U32 index, const vec4<U8>& newValue) {
        DIVIDE_ASSERT(index < _dataColor.size(),
                      "VertexBuffer error: Invalid color offset!");
        _dataColor[index].set(newValue);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_COLOR)] = true;
    }

    inline void modifyNormalValue(U32 index, const vec3<F32>& newValue) {
        DIVIDE_ASSERT(index < _dataNormal.size(),
                      "VertexBuffer error: Invalid normal offset!");
        _dataNormal[index].set(newValue);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_NORMAL)] = true;
    }

    inline void modifyTangentValue(U32 index, const vec3<F32>& newValue) {
        DIVIDE_ASSERT(index < _dataTangent.size(),
                      "VertexBuffer error: Invalid tangent offset!");
        _dataTangent[index].set(newValue);
        _attribDirty[to_uint(VertexAttribute::ATTRIB_TANGENT)] = true;
    }

    inline void shrinkAllDataToFit() {
        shrinkToFit(_dataPosition);
        shrinkToFit(_dataColor);
        shrinkToFit(_dataNormal);
        shrinkToFit(_dataTexcoord);
        shrinkToFit(_dataTangent);
        shrinkToFit(_boneWeights);
        shrinkToFit(_boneIndices);
    }

    inline size_t partitionBuffer(U32 currentIndexCount) {
        _partitions.push_back(std::make_pair(
            getIndexCount() - currentIndexCount, currentIndexCount));
        _currentPartitionIndex = to_uint(_partitions.size());
        return _currentPartitionIndex - 1;
    }

    inline U32 getPartitionCount(U16 partitionID) {
        if (_partitions.empty()) {
            return getIndexCount();
        }
        DIVIDE_ASSERT(partitionID < _partitions.size(),
                      "VertexBuffer error: Invalid partition offset!");
        return _partitions[partitionID].second;
    }

    inline U32 getPartitionOffset(U16 partitionID) {
        if (_partitions.empty()) {
            return 0;
        }
        DIVIDE_ASSERT(partitionID < _partitions.size(),
                      "VertexBuffer error: Invalid partition offset!");
        return _partitions[partitionID].first;
    }

    inline U32 getLastPartitionOffset() {
        if (_partitions.empty()) {
            return 0;
        }
        if (_partitions.empty()) return 0;
        return getPartitionOffset(static_cast<U16>(_partitions.size() - 1));
    }

    inline void reset() {
        _created = false;
        _primitiveRestartEnabled = false;
        _partitions.clear();
        _dataPosition.clear();
        _dataColor.clear();
        _dataNormal.clear();
        _dataTexcoord.clear();
        _dataTangent.clear();
        _boneIndices.clear();
        _boneWeights.clear();
        _hardwareIndicesL.clear();
        _hardwareIndicesS.clear();
        _attribDirty.fill(true);
        _VBoffset.fill(0);
    }

   protected:
    /// Just before we render the frame
    virtual bool frameStarted(const FrameEvent& evt) {
        return VertexDataInterface::frameStarted(evt);
    }

    virtual void checkStatus() = 0;
    virtual bool refresh() = 0;
    virtual bool createInternal() = 0;

   protected:
    /// Number of LOD nodes in this buffer
    U8 _LODcount;
    /// The format of the buffer data
    GFXDataFormat _format;
    /// An index value that separates objects (OGL: primitive restart index)
    U32 _indexDelimiter;
    std::array<ptrdiff_t, to_const_uint(VertexAttribute::COUNT)> _VBoffset;

    // first: offset, second: count
    vectorImpl<std::pair<U32, U32> > _partitions;
    /// Used for creating an "IB". If it's empty, then an outside source should
    /// provide the indices
    vectorImpl<U32> _hardwareIndicesL;
    vectorImpl<U16> _hardwareIndicesS;
    vectorImpl<vec3<F32> > _dataPosition;
    vectorImpl<vec4<U8>  > _dataColor;
    vectorImpl<vec3<F32> > _dataNormal;
    vectorImpl<vec2<F32> > _dataTexcoord;
    vectorImpl<vec3<F32> > _dataTangent;
    vectorImpl<P32 >       _boneIndices;
    vectorImpl<vec4<F32> > _boneWeights;
    /// Cache system to update only required data
    std::array<bool, to_const_uint(VertexAttribute::COUNT)> _attribDirty;
    bool _primitiveRestartEnabled;
    /// Was the data submitted to the GPU?
    bool _created;

   private:
    U32 _currentPartitionIndex;
    /// Use either U32 or U16 indices. Always prefer the later
    bool _largeIndices;
};

template<>
inline const vectorImpl<U32>& VertexBuffer::getIndices<U32>() const {
    return _hardwareIndicesL;
}

template<>
inline const vectorImpl<U16>& VertexBuffer::getIndices<U16>() const {
    return _hardwareIndicesS;
}
};  // namespace Divide
#endif
