/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _VERTEX_BUFFER_OBJECT_H
#define _VERTEX_BUFFER_OBJECT_H

#include <iostream>
#include "core.h"
enum PrimitiveType;
enum GFXDataFormat;
class ShaderProgram;
/// Vertex Buffer Object interface class to allow API-independent implementation of data
/// This class allow Vertex Array fallback if a VBO/VAO could not be generated
/// This class does NOT represent an API-level VBO, such as: GL_ARRAY_BUFFER / D3DVERTEXBUFFER
/// It is only a "buffer" for "vertex info" abstract of implementation. (e.g.: OGL uses a vertex array object for this)
class VertexBufferObject {

public:
    VertexBufferObject(PrimitiveType type) :
                            _positionDirty(true),
							_normalDirty(true),
							_texcoordDirty(true),
							_tangentDirty(true),
							_bitangentDirty(true),
							_indicesDirty(true),
							_weightsDirty(true),
							_staticDraw(true),
                            _useHWIndices(true),
                            _optimizeForDepth(true),
                            _forceOptimizeForDepth(false),
                            _depthPass(false),
							_currentShader(NULL),
							_VBOid(0),
                            _DepthVBOid(0),
							_IBOid(0),
							_LODcount(0)
	{
		_VBOoffsetPosition = 0;
		_VBOoffsetNormal = 0;
		_VBOoffsetTexcoord = 0;
		_VBOoffsetTangent = 0;
		_VBOoffsetBiTangent = 0;
	    _VBOoffsetBoneIndices = 0;
        _VBOoffsetBoneIndicesDEPTH = 0;
	    _VBOoffsetBoneWeights = 0;
        _VBOoffsetBoneWeightsDEPTH = 0;
        _type = type;
		_minPosition = vec3<F32>(10000.0f,10000.0f,10000.0f);
		_maxPosition = vec3<F32>(-10000.0f,-10000.0f,-10000.0f);
		for(U8 i = 0; i < SCENE_NODE_LOD; i++) _indiceLimits.push_back(vec2<U16>(0,0));
	}

   	virtual ~VertexBufferObject() {
		_VBOid = 0;
        _DepthVBOid = 0;
		_IBOid = 0;
		_LODcount = 0;
		_VBOoffsetPosition = 0;
		_VBOoffsetNormal = 0;
		_VBOoffsetTexcoord = 0;
		_VBOoffsetTangent = 0;
		_VBOoffsetBiTangent = 0;
		_dataPosition.clear();
		_dataNormal.clear();
		_dataTexcoord.clear();
		_dataTangent.clear();
		_dataBiTangent.clear();
		_boneIndices.clear();
		_boneWeights.clear();
		_hardwareIndices.clear();
		_dataTriangles.clear();
	};

	virtual bool Create(bool staticDraw = true) = 0;
	virtual void Destroy() = 0;
	
	virtual void Enable() = 0;
	virtual void Disable() = 0;
	virtual void Draw(U8 LODindex = 0) = 0;
    virtual void Draw(GFXDataFormat f, U32 count, const void* first_element) = 0;
	virtual void setShaderProgram(ShaderProgram* const shaderProgram) = 0;

	inline void setIndiceLimits(const vec2<U16> indiceLimits, U8 LODindex = 0) {assert(LODindex < _indiceLimits.size()); _indiceLimits[LODindex] = indiceLimits;}
	inline void reservePositionCount(U32 size) {_dataPosition.reserve(size);}
	inline void resizePositionCount(U32 size)  {_dataPosition.resize(size);}
	inline bool                     isShaderBased()  {return (_currentShader != NULL);}
	inline vectorImpl<U16>&         getHWIndices()   {return _hardwareIndices; }
	inline vectorImpl<vec3<F32> >&  getNormal()	     { _normalDirty    = true; return _dataNormal;}
	inline vectorImpl<vec2<F32> >&  getTexcoord()    { _texcoordDirty  = true; return _dataTexcoord;}
	inline vectorImpl<vec3<F32> >&  getTangent()	 { _tangentDirty   = true; return _dataTangent;}
	inline vectorImpl<vec3<F32> >&  getBiTangent()   { _bitangentDirty = true; return _dataBiTangent;}
	inline vectorImpl<vec4<U8>  >&  getBoneIndices() { _indicesDirty   = true; return _boneIndices;}
	inline vectorImpl<vec4<F32> >&  getBoneWeights() { _weightsDirty   = true; return _boneWeights;}
	inline vectorImpl<vec3<U32> >&  getTriangles()   { return _dataTriangles;}
	inline const vec3<F32>&         getMinPosition() {return _minPosition;}
	inline const vec3<F32>&         getMaxPosition() {return _maxPosition;}
    
	inline const vectorImpl<vec3<F32> >&  getPosition()	const {return _dataPosition;}
	inline const vec3<F32>&               getPosition(U32 index) const {return _dataPosition[index];}
    inline const PrimitiveType&           getPrimitiveType() const {return _type;}
	virtual bool queueRefresh() = 0;
	
	inline void addPosition(const vec3<F32>& pos){
		_positionDirty  = true; 
		if(pos.x > _maxPosition.x)	_maxPosition.x = pos.x;
		if(pos.x < _minPosition.x)	_minPosition.x = pos.x;
		if(pos.y > _maxPosition.y)	_maxPosition.y = pos.y;
		if(pos.y < _minPosition.y)	_minPosition.y = pos.y;
		if(pos.z > _maxPosition.z)	_maxPosition.z = pos.z;
		if(pos.z < _minPosition.z)	_minPosition.z = pos.z;
		_dataPosition.push_back(pos);
	}

	inline void modifyPositionValue(U32 index, const vec3<F32>& newValue){assert(index < _dataPosition.size()); _dataPosition[index] = newValue;}
    inline void useHWIndices(bool state = true) {_useHWIndices = state;}
    inline void optimizeForDepth(bool state = true,bool force = false) {_optimizeForDepth = state; _forceOptimizeForDepth = force;}
    inline void setDepthPass(bool state = false) {if(_optimizeForDepth) _depthPass = state;}

protected:
	virtual bool CreateInternal() = 0;
	virtual bool Refresh() = 0;
	virtual void checkStatus() = 0;
    virtual bool computeTriangleList() = 0;
protected:
	
	U32  		_VBOid;
    U32         _DepthVBOid;
	U32         _IBOid;
	U8          _LODcount; ///<Number of LOD nodes in this buffer
	ptrdiff_t	_VBOoffsetPosition;
	ptrdiff_t	_VBOoffsetNormal;
	ptrdiff_t	_VBOoffsetTexcoord;
	ptrdiff_t	_VBOoffsetTangent;
	ptrdiff_t	_VBOoffsetBiTangent;
	ptrdiff_t   _VBOoffsetBoneIndices;
    ptrdiff_t   _VBOoffsetBoneIndicesDEPTH;
	ptrdiff_t   _VBOoffsetBoneWeights;
    ptrdiff_t   _VBOoffsetBoneWeightsDEPTH;
	vectorImpl<vec2<U16> > _indiceLimits;
	///Used for creatinga "IBO". If it's empty, then an outside source should provide the indices
	vectorImpl<U16>        _hardwareIndices;
	vectorImpl<vec3<F32> > _dataPosition;
	vectorImpl<vec3<F32> > _dataNormal;
	vectorImpl<vec2<F32> > _dataTexcoord;
	vectorImpl<vec3<F32> > _dataTangent;
	vectorImpl<vec3<F32> > _dataBiTangent;
	vectorImpl<vec4<U8>  > _boneIndices;
	vectorImpl<vec4<F32> > _boneWeights;
	///3 indices, pointing to position values, that form a triangle in the mesh.
	vectorImpl<vec3<U32> > _dataTriangles; 
	vec3<F32> _minPosition;
	vec3<F32> _maxPosition;
	/// To change this, call Create again on the VBO
	/// Update calls use this as reference!
	bool _staticDraw;
    ///Some entities use their own indices for rendering (e.g. Terrain LOD system)
    bool _useHWIndices;
	/// Cache system to update only required data
	bool _positionDirty;
	bool _normalDirty;
	bool _texcoordDirty;
	bool _tangentDirty;
	bool _bitangentDirty;
	bool _indicesDirty;
	bool _weightsDirty;
    ///Store position and animation data in different VBO so rendering to depth is faster (it skips tex coords, tangent and bitangent data upload to GPU mem)
    ///Set this to FALSE if you need bump/normal/parallax mapping in depth pass (normal mapped object casts correct shadows)
    bool _optimizeForDepth;
    bool _forceOptimizeForDepth; //<Override vbo requirements for optimization
    ///If it's true, use depth only VBO/VAO else use the regular buffers
    bool _depthPass;
	///Used for VertexAttribPointer data. If shader is null, old style VBO/VA is used (TexCoordPointer for example)
	ShaderProgram* _currentShader;
    ///The format the data is in (TRIANGLES, TRIANGLE_STRIP,QUADS,etc)
    PrimitiveType  _type;
	
};

#endif

