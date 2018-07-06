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
class ShaderProgram;
/// Vertex Buffer Object interface class to allow API-independent implementation of data
/// This class allow Vertex Array fallback if a VBO/VAO could not be generated
/// This class does NOT represent an API-level VBO, such as: GL_ARRAY_BUFFER / D3DVERTEXBUFFER
/// It is only a "buffer" for "vertex info" abstract of implementation. (e.g.: OGL uses a vertex array object for this)
class VertexBufferObject {

public:
	virtual bool Create(bool staticDraw = true) = 0;
	virtual void Destroy() = 0;
	
	virtual void Enable() = 0;
	virtual void Disable() = 0;
	virtual void Draw(PrimitiveType type, U8 LODindex = 0) = 0;
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
	inline const vec3<F32>&         getMinPosition() {return _minPosition;}
	inline const vec3<F32>&         getMaxPosition() {return _maxPosition;}

	inline const vectorImpl<vec3<F32> >&  getPosition()	const {return _dataPosition;}
	inline const vec3<F32>&               getPosition(U32 index)	const {return _dataPosition[index];}

	/*inline void updatePosition(const vectorImpl<vec3<F32> >& newPositionData) {
		_dataPosition.clear(); 
		_dataPosition = newPositionData;
		_positionDirty = true;
	}

	inline void updateNormal(const vectorImpl<vec3<F32> >& newNormalData) {
		_dataNormal.clear(); 
		_dataNormal = newNormalData;
		_normalDirty = true;
	}

	inline void updateTexcoord(const vectorImpl<vec2<F32> >& newTexcoordData) {
		_dataTexcoord.clear(); 
		_dataTexcoord = newTexcoordData;
		_texcoordDirty = true;
	}

	inline void updateTangent(const vectorImpl<vec3<F32> >& newTangentData) {
		_dataTangent.clear(); 
		_dataTangent = newTangentData;
		_tangentDirty = true;
	}

	inline void updateBitangent(const vectorImpl<vec3<F32> >& newBiTangentData) {
		_dataBiTangent.clear(); 
		_dataBiTangent = newBiTangentData;
		_bitangentDirty = true;
	}

	inline void updateBoneIndices(const vectorImpl<vec4<U8> >& newBoneIndicesData) {
		_boneIndices.clear(); 
		_boneIndices = newBoneIndicesData;
		_indicesDirty = true;
	}

	inline void updateBoneWeights(const vectorImpl<vec4<F32> >& newBoneWeightsData) {
		_boneWeights.clear(); 
		_boneWeights = newBoneWeightsData;
		_weightsDirty = true;
	}*/

	virtual ~VertexBufferObject() {
		_VBOid = 0;
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
	};

	VertexBufferObject() : 	_positionDirty(true),
							_normalDirty(true),
							_texcoordDirty(true),
							_tangentDirty(true),
							_bitangentDirty(true),
							_indicesDirty(true),
							_weightsDirty(true),
							_staticDraw(true),
							_currentShader(NULL),
							_VBOid(0),
							_IBOid(0),
							_LODcount(0)
	{
		_VBOoffsetPosition = 0;
		_VBOoffsetNormal = 0;
		_VBOoffsetTexcoord = 0;
		_VBOoffsetTangent = 0;
		_VBOoffsetBiTangent = 0;
	    _VBOoffsetBoneIndices = 0;
	    _VBOoffsetBoneWeights = 0;
		_minPosition = vec3<F32>(1000.0f,1000.0f,1000.0f);
		_maxPosition = vec3<F32>(-1000.0f,-1000.0f,-1000.0f);
		for(U8 i = 0; i < SCENE_NODE_LOD; i++) _indiceLimits.push_back(vec2<U16>(0,0));
	}

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

protected:
	virtual bool CreateInternal() = 0;
	virtual bool Refresh() = 0;
	virtual void checkStatus() = 0;
protected:
	
	U32  		_VBOid;
	U32         _IBOid;
	U8          _LODcount; ///<Number of LOD nodes in this buffer
	ptrdiff_t	_VBOoffsetPosition;
	ptrdiff_t	_VBOoffsetNormal;
	ptrdiff_t	_VBOoffsetTexcoord;
	ptrdiff_t	_VBOoffsetTangent;
	ptrdiff_t	_VBOoffsetBiTangent;
	ptrdiff_t   _VBOoffsetBoneIndices;
	ptrdiff_t   _VBOoffsetBoneWeights;
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
	
	vec3<F32> _minPosition;
	vec3<F32> _maxPosition;
	/// To change this, call Create again on the VBO
	/// Update calls use this as reference!
	bool _staticDraw;

	/// Cache system to update only required data
	bool _positionDirty;
	bool _normalDirty;
	bool _texcoordDirty;
	bool _tangentDirty;
	bool _bitangentDirty;
	bool _indicesDirty;
	bool _weightsDirty;
	///Used for VertexAttribPointer data. If shader is null, old style VBO/VA is used (TexCoordPointer for example)
	ShaderProgram* _currentShader;
	
};

#endif

