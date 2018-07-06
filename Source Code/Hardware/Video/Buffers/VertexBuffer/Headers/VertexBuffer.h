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

#ifndef _VERTEX_BUFFER_OBJECT_H
#define _VERTEX_BUFFER_OBJECT_H

#include <iostream>
#include "Utility/Headers/Vector.h"
#include "Utility/Headers/GUIDWrapper.h"
#include "Core/Math/Headers/MathClasses.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"

enum PrimitiveType;
enum GFXDataFormat;
class ShaderProgram;
/// Vertex Buffer interface class to allow API-independent implementation of data
/// This class does NOT represent an API-level VB, such as: GL_ARRAY_BUFFER / D3DVERTEXBUFFER
/// It is only a "buffer" for "vertex info" abstract of implementation. (e.g.: OGL uses a vertex array object for this)
class VertexBuffer : public GUIDWrapper {
protected:
    struct IndirectDrawCommand {
        IndirectDrawCommand() : count(0), instanceCount(1), firstIndex(0), baseVertex(0), baseInstance(0) {}
        U32  count;
        U32  instanceCount;
        U32  firstIndex;
        U32  baseVertex;
        U32  baseInstance;
    };

    enum VertexAttribute{
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
    struct DeferredDrawCommand {
        DeferredDrawCommand() : _signedData(0), _unsignedData(0), _floatData(0.0f), _lodIndex(0) {}

        IndirectDrawCommand _cmd;
        I32                 _signedData;
        U32                 _unsignedData;
        F32                 _floatData;
        U8                  _lodIndex;
    };

    VertexBuffer(const PrimitiveType& type) : GUIDWrapper(),
                            _type(type),
                            _computeTriangles(true),
                            _largeIndices(false),
                            _format(UNSIGNED_SHORT),
                            _currentShader(nullptr),
                            _primitiveRestartEnabled(false),
                            _indexDelimiter(0),
                            _currentPartitionIndex(0)
    {
        _LODcount = 0;
        Reset();
    }

    virtual ~VertexBuffer()
    {
        _LODcount = 1;
        _currentShader = nullptr;
        Reset();
    }

    virtual bool Create(bool staticDraw = true) = 0;
    virtual void Destroy() = 0;
    /// Some engine elements, like physics or some geometry shading techniques require a triangle list.
    virtual bool computeTriangleList() = 0;
    virtual bool SetActive() = 0;

    virtual void Draw(const DeferredDrawCommand& command, bool skipBind = false) = 0;
    virtual void Draw(const vectorImpl<DeferredDrawCommand>& commands, bool skipBind = false) = 0;

    inline void setShaderProgram(ShaderProgram* const shaderProgram) { _currentShader = shaderProgram; }

    inline ShaderProgram* const currentShader()  {return _currentShader;}

    inline void setLODCount(const U8 LODcount)               {_LODcount = LODcount;}
    inline void useLargeIndices(bool state = true)           {
        DIVIDE_ASSERT(!_created, "VertexBuffer error: Indice format type specified before buffer creation!");
        _largeIndices = state; _format = _largeIndices ? UNSIGNED_INT : UNSIGNED_SHORT;
    }
    inline void computeTriangles(bool state = true)          {_computeTriangles = state;}
    inline void reservePositionCount(U32 size)  {_dataPosition.reserve(size);}
    inline void reserveColourCount(U32 size)    {_dataColor.reserve(size);}
    inline void reserveNormalCount(U32 size)    {_dataNormal.reserve(size);}
    inline void reserveTangentCount(U32 size)   {_dataTangent.reserve(size);}
    inline void reserveBiTangentCount(U32 size) {_dataBiTangent.reserve(size);}
    inline void reserveIndexCount(U32 size)     {_largeIndices ? _hardwareIndicesL.reserve(size) :_hardwareIndicesS.reserve(size);}
    inline void reserveTriangleCount(U32 size)  {_dataTriangles.reserve(size);}

    inline void resizePositionCount(U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO)  {
        _dataPosition.resize(size,defaultValue);
    }

    inline void resizeColoCount(U32 size, const vec3<U8>& defaultValue = vec3<U8>())           {
        _dataColor.resize(size,defaultValue);
    }

    inline void resizeNormalCount(U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO)    {
        _dataNormal.resize(size,defaultValue);
    }

    inline void resizeTangentCount(U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO)   {
        _dataTangent.resize(size,defaultValue);
    }

    inline void resizeBiTangentCount(U32 size, const vec3<F32>& defaultValue = VECTOR3_ZERO) {
        _dataBiTangent.resize(size,defaultValue);
    }

    inline vectorImpl<vec2<F32> >&  getTexcoord()    { _attribDirty[ATTRIB_TEXCOORD] = true; return _dataTexcoord; }
    inline vectorImpl<vec4<U8>  >&  getBoneIndices() { _attribDirty[ATTRIB_BONE_INDICE] = true; return _boneIndices; }
    inline vectorImpl<vec4<F32> >&  getBoneWeights() { _attribDirty[ATTRIB_BONE_WEIGHT] = true; return _boneWeights; }

    inline const vectorImpl<vec3<U32> >&  getTriangles()   const {return _dataTriangles;}
    inline const vectorImpl<vec3<F32> >&  getPosition()	   const {return _dataPosition;}
    inline const vectorImpl<vec3<U8>  >&  getColor()       const {return _dataColor;}
    inline const vectorImpl<vec3<F32> >&  getNormal()	   const {return _dataNormal;}
    inline const vectorImpl<vec3<F32> >&  getTangent()	   const {return _dataTangent;}
    inline const vectorImpl<vec3<F32> >&  getBiTangent()   const {return _dataBiTangent;}
    inline const vec3<F32>&               getPosition(U32 index)  const {return _dataPosition[index];}
    inline const vec3<F32>&               getNormal(U32 index)    const {return _dataNormal[index];}
    inline const vec3<F32>&               getTangent(U32 index)   const {return _dataTangent[index];}
    inline const vec3<F32>&               getBiTangent(U32 index) const {return _dataBiTangent[index];}
    inline const PrimitiveType&           getPrimitiveType()      const {return _type;}
    virtual bool queueRefresh() = 0;

    inline bool usesLargeIndices()  const { return _largeIndices;}
    inline U32  getIndexCount()     const { return (U32)(_largeIndices ? _hardwareIndicesL.size() : _hardwareIndicesS.size());}
    inline U32  getIndex(U32 index) const { return _largeIndices ? _hardwareIndicesL[index] : _hardwareIndicesS[index];}

    inline void addIndex(U32 index){
        _largeIndices ? addIndexL(index) : addIndexS(static_cast<U16>(index));
    }

    inline void addIndexL(U32 index){
        _hardwareIndicesL.push_back(index);
    }

    inline void addIndexS(U16 index){
        _hardwareIndicesS.push_back(index);
    }

    inline void addRestartIndex(){
        _primitiveRestartEnabled = true;
        addIndex(_largeIndices ? Config::PRIMITIVE_RESTART_INDEX_L : Config::PRIMITIVE_RESTART_INDEX_S);
    }

    inline void addTriangle(const vec3<U32>& tri){
        _dataTriangles.push_back(tri);
    }

    inline void addPosition(const vec3<F32>& pos){
        setMinMaxPosition(pos);
        _dataPosition.push_back(pos);
        _attribDirty[ATTRIB_POSITION] = true;
    }

    inline void addColor(const vec3<U8>& col){
        _dataColor.push_back(col);
        _attribDirty[ATTRIB_COLOR] = true;
    }

    inline void addTexCoord(const vec2<F32>& texCoord){
        _dataTexcoord.push_back(texCoord);
        _attribDirty[ATTRIB_TEXCOORD] = true;
    }

    inline void addNormal(const vec3<F32>& norm){
        _dataNormal.push_back(norm);
        _attribDirty[ATTRIB_NORMAL] = true;
    }

    inline void addTangent(const vec3<F32>& tangent){
        _dataTangent.push_back(tangent);
        _attribDirty[ATTRIB_TANGENT] = true;
    }

    inline void addBiTangent(const vec3<F32>& bitangent){
        _dataBiTangent.push_back(bitangent);
        _attribDirty[ATTRIB_BITANGENT] = true;
    }

    inline void addBoneIndex(const vec4<U8>& idx){
        _boneIndices.push_back(idx);
        _attribDirty[ATTRIB_BONE_INDICE] = true;
    }

    inline void addBoneWeight(const vec4<F32>& idx){
        _boneWeights.push_back(idx);
        _attribDirty[ATTRIB_BONE_WEIGHT] = true;
    }

    inline void setIndiceLimits(const vec2<U32>& indiceLimits, U8 LODindex = 0) {
        DIVIDE_ASSERT(LODindex < Config::SCENE_NODE_LOD, "VertexBuffer error: Invalid LOD passed for indices limits creation");
        vec2<U32>& limits = _indiceLimits[LODindex][_currentPartitionIndex];
        limits.y = std::max(indiceLimits.y, limits.y);
        limits.x = std::min(indiceLimits.x, limits.x);
    }

    inline void modifyPositionValue(U32 index, const vec3<F32>& newValue){
        DIVIDE_ASSERT(index < _dataPosition.size(), "VertexBuffer error: Invalid position offset!");
        _dataPosition[index] = newValue;
        _attribDirty[ATTRIB_POSITION] = true;
    }

    inline void modifyColorValue(U32 index, const vec3<U8>& newValue) {
        DIVIDE_ASSERT(index < _dataColor.size(), "VertexBuffer error: Invalid color offset!");
        _dataColor[index] = newValue;
        _attribDirty[ATTRIB_COLOR] = true;
    }

    inline void modifyNormalValue(U32 index, const vec3<F32>& newValue)  {
        DIVIDE_ASSERT(index < _dataNormal.size(), "VertexBuffer error: Invalid normal offset!");
        _dataNormal[index] = newValue;
        _attribDirty[ATTRIB_NORMAL] = true;
    }

    inline void modifyTangentValue(U32 index, const vec3<F32>& newValue)  {
        DIVIDE_ASSERT(index < _dataTangent.size(), "VertexBuffer error: Invalid tangent offset!");
        _dataTangent[index] = newValue;
        _attribDirty[ATTRIB_TANGENT] = true;
    }

    inline void modifyBiTangentValue(U32 index, const vec3<F32>& newValue)  {
        DIVIDE_ASSERT(index < _dataBiTangent.size(), "VertexBuffer error: Invalid bitangent offset!");
        _dataBiTangent[index] = newValue;
        _attribDirty[ATTRIB_BITANGENT] = true;
    }

    inline size_t partitionBuffer(U32 currentIndexCount){
        _partitions.push_back(std::make_pair(getIndexCount() - currentIndexCount, currentIndexCount));
        _currentPartitionIndex = (U32)_partitions.size();
        _minPosition.push_back(vec3<F32>(std::numeric_limits<F32>::max()));
        _maxPosition.push_back(vec3<F32>(std::numeric_limits<F32>::min()));
        for(U8 i = 0; i < Config::SCENE_NODE_LOD; ++i){
            _indiceLimits[i].push_back(vec2<U32>(0,0));
        }
        return _currentPartitionIndex - 1;
    }

    inline U32 getPartitionCount(U16 partitionIdx){
        DIVIDE_ASSERT(partitionIdx < _partitions.size(), "VertexBuffer error: Invalid partition offset!");
        return _partitions[partitionIdx].second;
    }

    inline U32 getPartitionOffset(U16 partitionIdx){
        DIVIDE_ASSERT(partitionIdx < _partitions.size(), "VertexBuffer error: Invalid partition offset!");
        return _partitions[partitionIdx].first;
    }

    inline U32 getLastPartitionOffset(){
        if (_partitions.empty()) return 0;
        return getPartitionOffset((U16)(_partitions.size() - 1));
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
        _dataTriangles.clear();
        for(U8 i = 0; i < Config::SCENE_NODE_LOD; ++i){
            _indiceLimits[i].resize(1, vec2<U32>(0,0));
        }
        memset(_attribDirty, true, VertexAttribute_PLACEHOLDER * sizeof(bool));
        memset(_VBoffset, 0, VertexAttribute_PLACEHOLDER * sizeof(ptrdiff_t));
        _minPosition.resize(1, vec3<F32>(std::numeric_limits<F32>::max()));
        _maxPosition.resize(1, vec3<F32>(std::numeric_limits<F32>::min()));
        
    }

protected:
    virtual void checkStatus() = 0;
    virtual bool Refresh() = 0;
    virtual bool CreateInternal() = 0;

    inline void setMinMaxPosition(const vec3<F32>& pos){
        vec3<F32>& min = _minPosition[_currentPartitionIndex];
        vec3<F32>& max = _maxPosition[_currentPartitionIndex];

        if (pos.x > max.x)	max.x = pos.x;
        if (pos.x < min.x)	min.x = pos.x;
        if (pos.y > max.y)	max.y = pos.y;
        if (pos.y < min.y)	min.y = pos.y;
        if (pos.z > max.z)	max.z = pos.z;
        if (pos.z < min.z)	min.z = pos.z;
    }

protected:

    ///Number of LOD nodes in this buffer
    U8          _LODcount; 
    ///The format of the buffer data
    GFXDataFormat _format;
    ///An index value that separates ojects (OGL: primitive restart index)
    U32         _indexDelimiter;
    ptrdiff_t	_VBoffset[VertexAttribute_PLACEHOLDER];
        
    // first: offset, second: count
    vectorImpl<std::pair<U32, U32> >   _partitions;
    vectorImpl<vec2<U32> > _indiceLimits[Config::SCENE_NODE_LOD];
    ///Used for creating an "IB". If it's empty, then an outside source should provide the indices
    vectorImpl<U32>        _hardwareIndicesL;
    vectorImpl<U16>        _hardwareIndicesS;
    vectorImpl<vec3<F32> > _dataPosition;
    vectorImpl<vec3<U8>  > _dataColor;
    vectorImpl<vec3<F32> > _dataNormal;
    vectorImpl<vec2<F32> > _dataTexcoord;
    vectorImpl<vec3<F32> > _dataTangent;
    vectorImpl<vec3<F32> > _dataBiTangent;
    vectorImpl<vec4<U8>  > _boneIndices;
    vectorImpl<vec4<F32> > _boneWeights;
    vectorImpl<vec3<U32> > _dataTriangles;	//< 3 indices, pointing to position values, that form a triangle in the mesh.
    vectorImpl<vec3<F32> > _minPosition;
    vectorImpl<vec3<F32> > _maxPosition;
    ///Use either U32 or U16 indices. Always prefer the later
    bool _largeIndices;
    ///Some objects need triangle data in order for other parts of the engine to take advantage of direct data (physics, navmeshes, etc)
    bool _computeTriangles;
    /// Cache system to update only required data
    bool _attribDirty[VertexAttribute_PLACEHOLDER];
    bool _primitiveRestartEnabled;
    ///Was the data submited to the GPU?
    bool _created;
    ///Used for VertexAttribPointer data.
    ShaderProgram* _currentShader;
    ///The format the data is in (TRIANGLES, TRIANGLE_STRIP,QUADS,etc)
    PrimitiveType  _type;

private:
    U32 _currentPartitionIndex;
};

#endif
