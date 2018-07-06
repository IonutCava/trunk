/*“Copyright 2009-2012 DIVIDE-Studio”*/
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
#include "resource.h"

class ShaderProgram;
/// Vertex Buffer Object interface class  to allow API-independent implementation of data
/// This class allow Vertex Array fallback if a VBO could not be generated
class VertexBufferObject {

public:
	virtual bool Create(bool staticDraw = true) = 0;
	virtual void Destroy() = 0;
	
	virtual void Enable() = 0;
	virtual void Disable() = 0;

	virtual bool Refresh() = 0;

	virtual void setShaderProgram(ShaderProgram* const shaderProgram) = 0;
	inline  bool isShaderBased() {return (_currentShader != NULL);}

	inline std::vector<vec3<F32> >&  getPosition()	  { _positionDirty  = true; return _dataPosition;}
	inline std::vector<vec3<F32> >&  getNormal()	  { _normalDirty    = true; return _dataNormal;}
	inline std::vector<vec2<F32> >&  getTexcoord()    { _texcoordDirty  = true; return _dataTexcoord;}
	inline std::vector<vec3<F32> >&  getTangent()	  { _tangentDirty   = true; return _dataTangent;}
	inline std::vector<vec3<F32> >&  getBiTangent()   { _bitangentDirty = true; return _dataBiTangent;}
	inline std::vector<vec4<U8>  >&  getBoneIndices() { _indicesDirty   = true; return _boneIndices;}
	inline std::vector<vec4<F32> >&  getBoneWeights() { _weightsDirty   = true; return _boneWeights;}
	inline std::vector<U16>&         getHWIndices()   {return _hardwareIndices; }

	/*inline void updatePosition(const std::vector<vec3<F32> >& newPositionData) {
		_dataPosition.clear(); 
		_dataPosition = newPositionData;
		_positionDirty = true;
	}

	inline void updateNormal(const std::vector<vec3<F32> >& newNormalData) {
		_dataNormal.clear(); 
		_dataNormal = newNormalData;
		_normalDirty = true;
	}

	inline void updateTexcoord(const std::vector<vec2<F32> >& newTexcoordData) {
		_dataTexcoord.clear(); 
		_dataTexcoord = newTexcoordData;
		_texcoordDirty = true;
	}

	inline void updateTangent(const std::vector<vec3<F32> >& newTangentData) {
		_dataTangent.clear(); 
		_dataTangent = newTangentData;
		_tangentDirty = true;
	}

	inline void updateBitangent(const std::vector<vec3<F32> >& newBiTangentData) {
		_dataBiTangent.clear(); 
		_dataBiTangent = newBiTangentData;
		_bitangentDirty = true;
	}

	inline void updateBoneIndices(const std::vector<vec4<U8> >& newBoneIndicesData) {
		_boneIndices.clear(); 
		_boneIndices = newBoneIndicesData;
		_indicesDirty = true;
	}

	inline void updateBoneWeights(const std::vector<vec4<F32> >& newBoneWeightsData) {
		_boneWeights.clear(); 
		_boneWeights = newBoneWeightsData;
		_weightsDirty = true;
	}*/

	virtual ~VertexBufferObject() {
		_VBOid = 0;
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
							_IBOid(0)
	{
		_VBOoffsetPosition = 0;
		_VBOoffsetNormal = 0;
		_VBOoffsetTexcoord = 0;
		_VBOoffsetTangent = 0;
		_VBOoffsetBiTangent = 0;
	    _VBOoffsetBoneIndices = 0;
	    _VBOoffsetBoneWeights = 0;
	}


protected:
	virtual bool CreateInternal() = 0;

protected:
	
	U32  		_VBOid;
	U32         _IBOid;
	ptrdiff_t	_VBOoffsetPosition;
	ptrdiff_t	_VBOoffsetNormal;
	ptrdiff_t	_VBOoffsetTexcoord;
	ptrdiff_t	_VBOoffsetTangent;
	ptrdiff_t	_VBOoffsetBiTangent;
	ptrdiff_t   _VBOoffsetBoneIndices;
	ptrdiff_t   _VBOoffsetBoneWeights;

	
	///Used for creatinga "IBO". If it's empty, then an outside source should provide the indices
	std::vector<U16>        _hardwareIndices;
	std::vector<vec3<F32> >	_dataPosition;
	std::vector<vec3<F32> >	_dataNormal;
	std::vector<vec2<F32> >	_dataTexcoord;
	std::vector<vec3<F32> >	_dataTangent;
	std::vector<vec3<F32> >	_dataBiTangent;
	std::vector<vec4<U8>  > _boneIndices;
	std::vector<vec4<F32> > _boneWeights;

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

