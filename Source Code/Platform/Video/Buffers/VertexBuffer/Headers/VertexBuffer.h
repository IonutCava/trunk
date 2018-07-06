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
class VertexBuffer : public VertexDataInterface {
   protected:
    enum VertexAttribute {
        ATTRIB_POSITION = 0,
        ATTRIB_COLOR = 1,
        ATTRIB_NORMAL = 2,
        ATTRIB_TEXCOORD = 3,
        ATTRIB_TANGENT = 4,
        ATTRIB_BITANGENT = 5,
        ATTRIB_BONE_WEIGHT = 6,
        ATTRIB_BONE_INDICE = 7,
        VertexAttribute_PLACEHOLDER = 8
    };

   public:
    VertexBuffer()
        : VertexDataInterface(),
          _largeIndices(false),
          _format(GFXDataFormat::UNSIGNED_SHORT),
          _primitiveRestartEnabled(false),
          _indexDelimiter(0),
          _currentPartitionIndex(0) {
        _LODcount = 0;
        Reset();
    }

    virtual ~VertexBuffer() {
        _LODcount = 1;
        Reset();
    }

    virtual bool Create(bool staticDraw = true) = 0;
    virtual void Destroy() = 0;
    /// Some engine elements, like physics or some geometry shading techniques
    /// require a triangle list.
    virtual bool SetActive() = 0;

    virtual void Draw(const GenericDrawCommand& command,
                      bool skipBind = false) = 0;
    virtual void Draw(const vectorImpl<GenericDrawCommand>& commands,
                      bool skipBind = false) = 0;

    inline void setLODCount(const U8 LODcount) { _LODcount = LODcount; }

    inline void useLargeIndices(bool state = true) {
        DIVIDE_ASSERT(!_created,
                      "VertexBuffer error: Index format type specified before "
                      "buffer creation!");
        _largeIndices = state;
        _format = _largeIndices ? GFXDataFormat::UNSIGNED_INT
                                : GFXDataFormat::UNSIGNED_SHORT;
    }

    inline void reservePositionCount(U32 size) { _dataPosition.reserve(size); }
    inline void reserveColourCount(U32 size) { _dataColor.reserve(size); }
    inline void reserveNormalCount(U32 size) { _dataNormal.reserve(size); }
    inline void reserveTangentCount(U32 size) { _dataTangent.reserve(size); }
    inline void reserveBiTangentCount(U32 size) {
        _dataBiTangent.reserve(size);
    }

    inline void reserveIndexCount(U32 size) {
        _largeIndices ? _hardwareIndicesL.reserve(size)
                      : _hardwareIndicesS.reserve(size);
    }

    inline void resizePositionCount(
        U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO) {
        _dataPosition.resize(size, defaultValue);
    }

    inline void resizeColoCount(U32 size,
                                const vec3<U8>& defaultValue = vec3<U8>()) {
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

    inline void resizeBiTangentCount(
        U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO) {
        _dataBiTangent.resize(size, defaultValue);
    }

    inline vectorImpl<vec2<F32> >& getTexcoord() {
        _attribDirty[ATTRIB_TEXCOORD] = true;
        return _dataTexcoord;
    }
    inline vectorImpl<vec4<U8> >& getBoneIndices() {
        _attribDirty[ATTRIB_BONE_INDICE] = true;
        return _boneIndices;
    }
    inline vectorImpl<vec4<F32> >& getBoneWeights() {
        _attribDirty[ATTRIB_BONE_WEIGHT] = true;
        return _boneWeights;
    }

    inline const vectorImpl<vec3<F32> >& getPosition() const {
        return _dataPosition;
    }
    inline const vectorImpl<vec3<U8> >& getColor() const { return _dataColor; }
    inline const vectorImpl<vec3<F32> >& getNormal() const {
        return _dataNormal;
    }
    inline const vectorImpl<vec3<F32> >& getTangent() const {
        return _dataTangent;
    }
    inline const vectorImpl<vec3<F32> >& getBiTangent() const {
        return _dataBiTangent;
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
    inline const vec3<F32>& getBiTangent(U32 index) const {
        return _dataBiTangent[index];
    }
    virtual bool queueRefresh() = 0;

    inline bool usesLargeIndices() const { return _largeIndices; }
    inline U32 getIndexCount() const {
        return static_cast<U32>(_largeIndices ? _hardwareIndicesL.size()
                                              : _hardwareIndicesS.size());
    }
    inline U32 getIndex(U32 index) const {
        return _largeIndices ? _hardwareIndicesL[index]
                             : _hardwareIndicesS[index];
    }

    template<typename T>
    const vectorImpl<T>& getIndices() const {
        static_assert(false, "VertexBuffer::getIndices error: Need valid index data type!");
    }

    template<>
    const vectorImpl<U32>& getIndices() const {
        return _hardwareIndicesL;
    }

    template<>
    inline const vectorImpl<U16>& getIndices() const {
        return _hardwareIndicesS;
    }

    inline void addIndex(U32 index) {
        _largeIndices ? addIndexL(index) : addIndexS(static_cast<U16>(index));
    }

    inline void addIndexL(U32 index) { _hardwareIndicesL.push_back(index); }

    inline void addIndexS(U16 index) { _hardwareIndicesS.push_back(index); }

    inline void addRestartIndex() {
        _primitiveRestartEnabled = true;
        addIndex(_largeIndices ? Config::PRIMITIVE_RESTART_INDEX_L
                               : Config::PRIMITIVE_RESTART_INDEX_S);
    }

    inline void addPosition(const vec3<F32>& pos) {
        setMinMaxPosition(pos);
        _dataPosition.push_back(pos);
        _attribDirty[ATTRIB_POSITION] = true;
    }

    inline void addColor(const vec3<U8>& col) {
        _dataColor.push_back(col);
        _attribDirty[ATTRIB_COLOR] = true;
    }

    inline void addTexCoord(const vec2<F32>& texCoord) {
        _dataTexcoord.push_back(texCoord);
        _attribDirty[ATTRIB_TEXCOORD] = true;
    }

    inline void addNormal(const vec3<F32>& norm) {
        _dataNormal.push_back(norm);
        _attribDirty[ATTRIB_NORMAL] = true;
    }

    inline void addTangent(const vec3<F32>& tangent) {
        _dataTangent.push_back(tangent);
        _attribDirty[ATTRIB_TANGENT] = true;
    }

    inline void addBiTangent(const vec3<F32>& bitangent) {
        _dataBiTangent.push_back(bitangent);
        _attribDirty[ATTRIB_BITANGENT] = true;
    }

    inline void addBoneIndex(const vec4<U8>& idx) {
        _boneIndices.push_back(idx);
        _attribDirty[ATTRIB_BONE_INDICE] = true;
    }

    inline void addBoneWeight(const vec4<F32>& idx) {
        _boneWeights.push_back(idx);
        _attribDirty[ATTRIB_BONE_WEIGHT] = true;
    }

    inline void modifyPositionValue(U32 index, const vec3<F32>& newValue) {
        DIVIDE_ASSERT(index < _dataPosition.size(),
                      "VertexBuffer error: Invalid position offset!");
        _dataPosition[index] = newValue;
        _attribDirty[ATTRIB_POSITION] = true;
    }

    inline void modifyColorValue(U32 index, const vec3<U8>& newValue) {
        DIVIDE_ASSERT(index < _dataColor.size(),
                      "VertexBuffer error: Invalid color offset!");
        _dataColor[index] = newValue;
        _attribDirty[ATTRIB_COLOR] = true;
    }

    inline void modifyNormalValue(U32 index, const vec3<F32>& newValue) {
        DIVIDE_ASSERT(index < _dataNormal.size(),
                      "VertexBuffer error: Invalid normal offset!");
        _dataNormal[index] = newValue;
        _attribDirty[ATTRIB_NORMAL] = true;
    }

    inline void modifyTangentValue(U32 index, const vec3<F32>& newValue) {
        DIVIDE_ASSERT(index < _dataTangent.size(),
                      "VertexBuffer error: Invalid tangent offset!");
        _dataTangent[index] = newValue;
        _attribDirty[ATTRIB_TANGENT] = true;
    }

    inline void modifyBiTangentValue(U32 index, const vec3<F32>& newValue) {
        DIVIDE_ASSERT(index < _dataBiTangent.size(),
                      "VertexBuffer error: Invalid bitangent offset!");
        _dataBiTangent[index] = newValue;
        _attribDirty[ATTRIB_BITANGENT] = true;
    }

    inline void shrinkAllDataToFit() {
        shrinkToFit(_dataPosition);
        shrinkToFit(_dataColor);
        shrinkToFit(_dataNormal);
        shrinkToFit(_dataTexcoord);
        shrinkToFit(_dataTangent);
        shrinkToFit(_dataBiTangent);
        shrinkToFit(_boneWeights);
        shrinkToFit(_boneIndices);
    }

    inline size_t partitionBuffer(U32 currentIndexCount) {
        _partitions.push_back(vectorAlg::makePair(
            getIndexCount() - currentIndexCount, currentIndexCount));
        _currentPartitionIndex = (U32)_partitions.size();
        _minPosition.push_back(std::numeric_limits<F32>::max());
        _maxPosition.push_back(std::numeric_limits<F32>::min());
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

    inline void Reset() {
        _created = false;
        _primitiveRestartEnabled = false;
        _partitions.clear();
        _dataPosition.clear();
        _dataColor.clear();
        _dataNormal.clear();
        _dataTexcoord.clear();
        _dataTangent.clear();
        _dataBiTangent.clear();
        _boneIndices.clear();
        _boneWeights.clear();
        _hardwareIndicesL.clear();
        _hardwareIndicesS.clear();
        memset(_attribDirty, true, VertexAttribute_PLACEHOLDER * sizeof(bool));
        memset(_VBoffset, 0, VertexAttribute_PLACEHOLDER * sizeof(ptrdiff_t));
        _minPosition.resize(1, vec3<F32>(std::numeric_limits<F32>::max()));
        _maxPosition.resize(1, vec3<F32>(std::numeric_limits<F32>::min()));
    }

   protected:
    /// Just before we render the frame
    virtual bool frameStarted(const FrameEvent& evt) {
        return VertexDataInterface::frameStarted(evt);
    }

    virtual void checkStatus() = 0;
    virtual bool Refresh() = 0;
    virtual bool CreateInternal() = 0;

    inline void setMinMaxPosition(const vec3<F32>& pos) {
        vec3<F32>& min = _minPosition[_currentPartitionIndex];
        vec3<F32>& max = _maxPosition[_currentPartitionIndex];

        if (pos.x > max.x) max.x = pos.x;
        if (pos.x < min.x) min.x = pos.x;
        if (pos.y > max.y) max.y = pos.y;
        if (pos.y < min.y) min.y = pos.y;
        if (pos.z > max.z) max.z = pos.z;
        if (pos.z < min.z) min.z = pos.z;
    }

   protected:
    /// Number of LOD nodes in this buffer
    U8 _LODcount;
    /// The format of the buffer data
    GFXDataFormat _format;
    /// An index value that separates objects (OGL: primitive restart index)
    U32 _indexDelimiter;
    ptrdiff_t _VBoffset[VertexAttribute_PLACEHOLDER];

    // first: offset, second: count
    vectorImpl<vectorAlg::pair<U32, U32> > _partitions;
    /// Used for creating an "IB". If it's empty, then an outside source should
    /// provide the indices
    vectorImpl<U32> _hardwareIndicesL;
    vectorImpl<U16> _hardwareIndicesS;
    vectorImpl<vec3<F32> > _dataPosition;
    vectorImpl<vec3<U8> > _dataColor;
    vectorImpl<vec3<F32> > _dataNormal;
    vectorImpl<vec2<F32> > _dataTexcoord;
    vectorImpl<vec3<F32> > _dataTangent;
    vectorImpl<vec3<F32> > _dataBiTangent;
    vectorImpl<vec4<U8> > _boneIndices;
    vectorImpl<vec4<F32> > _boneWeights;
    vectorImpl<vec3<F32> > _minPosition;
    vectorImpl<vec3<F32> > _maxPosition;
    /// Use either U32 or U16 indices. Always prefer the later
    bool _largeIndices;
    /// Cache system to update only required data
    bool _attribDirty[VertexAttribute_PLACEHOLDER];
    bool _primitiveRestartEnabled;
    /// Was the data submitted to the GPU?
    bool _created;

   private:
    U32 _currentPartitionIndex;
};

};  // namespace Divide
#endif
