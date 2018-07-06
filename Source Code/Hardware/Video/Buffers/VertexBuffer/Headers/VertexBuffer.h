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
#include <cassert>
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
public:
    VertexBuffer(const PrimitiveType& type) : GUIDWrapper(),
                            _type(type),
                            _computeTriangles(true),
                            _largeIndices(false),
                            _format(UNSIGNED_SHORT),
                            _currentShader(nullptr),
                            _primitiveRestartEnabled(false),
                            _firstElement(0),
                            _instanceCount(1),
                            _indexDelimiter(0)
    {
        _depthPass = _forceOptimizeForDepth = false;
        _LODcount = _rangeCount = 0;
        _optimizeForDepth = true;
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

    virtual void Draw(bool skipBind = false, const U8 LODindex = 0) = 0;
    virtual void DrawRange(bool skipBind = false) = 0;

    virtual void setShaderProgram(ShaderProgram* const shaderProgram) = 0;
    inline ShaderProgram* const currentShader()  {return _currentShader;}

    inline void setLODCount(const U8 LODcount)               {_LODcount = LODcount;}
    inline void setInstanceCount(const U32 instanceCount)    {_instanceCount = instanceCount;}
    inline void setRangeCount(const U32 rangeCount)          {_rangeCount = rangeCount;}
    inline void setFirstElement(U32 firstElement)            {_firstElement = firstElement;}
    inline void setDepthPass(bool state = false)             {if(_optimizeForDepth) _depthPass = state;}
    inline void useLargeIndices(bool state = true)           {assert(!_created); _largeIndices = state; _format = _largeIndices ? UNSIGNED_INT : UNSIGNED_SHORT;}
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

    inline vectorImpl<vec2<F32> >&  getTexcoord()    { _texcoordDirty  = true; return _dataTexcoord;}
    inline vectorImpl<vec4<U8>  >&  getBoneIndices() { _indicesDirty   = true; return _boneIndices;}
    inline vectorImpl<vec4<F32> >&  getBoneWeights() { _weightsDirty   = true; return _boneWeights;}

    inline vectorImpl<vec3<U32> >&  getTriangles()   {return _dataTriangles;}
    inline const vec3<F32>&         getMinPosition() {return _minPosition;}
    inline const vec3<F32>&         getMaxPosition() {return _maxPosition;}

    inline const vectorImpl<vec3<F32> >&  getPosition()	 const {return _dataPosition;}
    inline const vectorImpl<vec3<U8>  >&  getColor()     const {return _dataColor;}
    inline const vectorImpl<vec3<F32> >&  getNormal()	 const {return _dataNormal;}
    inline const vectorImpl<vec3<F32> >&  getTangent()	 const {return _dataTangent;}
    inline const vectorImpl<vec3<F32> >&  getBiTangent() const {return _dataBiTangent;}

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
        if(_largeIndices){
            _hardwareIndicesL.push_back(index);
        }else{
            _hardwareIndicesS.push_back(static_cast<U16>(index));
        }
    }

    inline void addIndexL(U32 index){
        _hardwareIndicesL.push_back(index);
    }

    inline void addIndexS(U16 index){
        _hardwareIndicesS.push_back(index);
    }

    inline void addRestartIndex(){
        _primitiveRestartEnabled = true;
        if (_largeIndices){
            _hardwareIndicesL.push_back(Config::PRIMITIVE_RESTART_INDEX_L);
        }else{
            _hardwareIndicesS.push_back(Config::PRIMITIVE_RESTART_INDEX_S);
        }
    }
    inline void addPosition(const vec3<F32>& pos){
        if(pos.x > _maxPosition.x)	_maxPosition.x = pos.x;
        if(pos.x < _minPosition.x)	_minPosition.x = pos.x;
        if(pos.y > _maxPosition.y)	_maxPosition.y = pos.y;
        if(pos.y < _minPosition.y)	_minPosition.y = pos.y;
        if(pos.z > _maxPosition.z)	_maxPosition.z = pos.z;
        if(pos.z < _minPosition.z)	_minPosition.z = pos.z;
        _dataPosition.push_back(pos);
        _positionDirty  = true;
    }

    inline void addColor(const vec3<U8>& col){
        _dataColor.push_back(col);
        _colorDirty = true;
    }

    inline void addNormal(const vec3<F32>& norm){
        _dataNormal.push_back(norm);
        _normalDirty = true;
    }

    inline void addTangent(const vec3<F32>& tangent){
        _dataTangent.push_back(tangent);
        _tangentDirty = true;
    }

    inline void addBiTangent(const vec3<F32>& bitangent){
        _dataBiTangent.push_back(bitangent);
        _bitangentDirty = true;
    }

    inline void setIndiceLimits(const vec2<U32>& indiceLimits, U8 LODindex = 0) {
        assert(LODindex < _indiceLimits.size());
       _indiceLimits[LODindex] = indiceLimits;
    }

    inline void modifyPositionValue(U32 index, const vec3<F32>& newValue){
        assert(index < _dataPosition.size());
        _dataPosition[index] = newValue;
        _positionDirty = true;
    }

    inline void modifyColorValue(U32 index, const vec3<U8>& newValue) {
        assert(index < _dataColor.size());
        _dataColor[index] = newValue;
        _colorDirty = true;
    }

    inline void modifyNormalValue(U32 index, const vec3<F32>& newValue)  {
        assert(index < _dataNormal.size());
        _dataNormal[index] = newValue;
        _normalDirty = true;
    }

    inline void modifyTangentValue(U32 index, const vec3<F32>& newValue)  {
        assert(index < _dataTangent.size());
        _dataTangent[index] = newValue;
        _tangentDirty = true;
    }

    inline void modifyBiTangentValue(U32 index, const vec3<F32>& newValue)  {
        assert(index < _dataBiTangent.size());
        _dataBiTangent[index] = newValue;
        _bitangentDirty = true;
    }

    inline void optimizeForDepth(bool state = true,bool force = false) {
        _optimizeForDepth = state;
        _forceOptimizeForDepth = force;
    }

    inline void Reset() {
        _created = false;
        _primitiveRestartEnabled = false;
        _VBoffsetPosition = _VBoffsetColor = _VBoffsetNormal = _VBoffsetTexcoord = _VBoffsetTangent = 0;
        _VBoffsetBiTangent = _VBoffsetBoneIndices = _VBoffsetBoneWeights = 0;
        _VBoffsetBoneIndicesDEPTH = _VBoffsetBoneWeightsDEPTH = 0;
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
        _indiceLimits.resize(Config::SCENE_NODE_LOD, vec2<U32>(0,0));
        _positionDirty = _colorDirty = _normalDirty = _texcoordDirty = true;
        _tangentDirty = _bitangentDirty = _indicesDirty = _weightsDirty = true;
        _minPosition = vec3<F32>(10000.0f);
        _maxPosition = vec3<F32>(-10000.0f);
    }

protected:
    virtual void checkStatus() = 0;
    virtual bool Refresh() = 0;
    virtual bool CreateInternal() = 0;

protected:

    ///Number of LOD nodes in this buffer
    U8          _LODcount; 
    ///How many elements should we actually render when using "DrawRange"
    U32         _rangeCount;
    ///The format of the buffer data
    GFXDataFormat _format;
    ///Number of instances to draw
    U32         _instanceCount;
    ///An index value that separates ojects (OGL: primitive restart index)
    U32         _indexDelimiter;
    ptrdiff_t	_VBoffsetPosition;
    ptrdiff_t   _VBoffsetColor;
    ptrdiff_t	_VBoffsetNormal;
    ptrdiff_t	_VBoffsetTexcoord;
    ptrdiff_t	_VBoffsetTangent, _VBoffsetBiTangent;
    ptrdiff_t   _VBoffsetBoneIndices, _VBoffsetBoneWeights;
    ptrdiff_t   _VBoffsetBoneIndicesDEPTH, _VBoffsetBoneWeightsDEPTH;

    vectorImpl<vec2<U32> > _indiceLimits;
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
    vec3<F32> _minPosition,  _maxPosition;
    ///Use ither U32 or U16 indices. Always prefer the later
    bool _largeIndices;
    ///Some objects need triangle data in order for other parts of the engine to take advantage of direct data (physics, navmeshes, etc)
    bool _computeTriangles;
    /// Cache system to update only required data
    bool _positionDirty, _colorDirty, _normalDirty, _texcoordDirty;
    bool _tangentDirty, _bitangentDirty, _indicesDirty, _weightsDirty;
    ///Store position and animation data in different VB so rendering to depth is faster (it skips tex coords, tangent and bitangent data upload to GPU mem)
    ///Set this to FALSE if you need bump/normal/parallax mapping in depth pass (normal mapped object casts correct shadows)
    bool _optimizeForDepth;
    bool _forceOptimizeForDepth; //<Override vb requirements for optimization
    bool _primitiveRestartEnabled;
    ///If it's true, use depth only VB/VAO else use the regular buffers
    bool _depthPass;
    ///Was the data submited to the GPU?
    bool _created;
    ///Used for VertexAttribPointer data.
    ShaderProgram* _currentShader;
    ///Offset to the first element in the buffer
    U32 _firstElement;
    ///The format the data is in (TRIANGLES, TRIANGLE_STRIP,QUADS,etc)
    PrimitiveType  _type;
};

#endif