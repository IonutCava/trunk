/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

struct vertexWeight {
	U32 _vertexId;
	F32 _weight;

#ifdef __cplusplus

	vertexWeight() { }
	vertexWeight( U32 vID, F32 vWeight) 
		: _vertexId( vID), _weight( vWeight) 
	{ }

#endif
};

class VertexBufferObject {

public:
	virtual bool Create() = 0;
	virtual bool Create(U32 usage) = 0;
	virtual void Destroy() = 0;
	
	virtual void Enable() = 0;
	virtual void Disable() = 0;

	
	inline std::vector<vec3>&	getPosition()	{return _dataPosition;}
	inline std::vector<vec3>&	getNormal()		{return _dataNormal;}
	inline std::vector<vec2>&	getTexcoord()	{return _dataTexcoord;}
	inline std::vector<vec3>&	getTangent()	{return _dataTangent;}
	inline std::vector<vec3>&	getBiTangent()	{return _dataBiTangent;}
	inline std::vector<std::vector<U8>>& getBoneIndices() {return _boneIndices;}
	inline std::vector<std::vector<U8>>& getBoneWeights() {return _boneWeights;}

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
		for(U16 i = 0; i < _boneIndices.size(); i++)
			_boneIndices[i].clear();
		_boneIndices.clear();
		for(U16 i = 0; i < _boneWeights.size(); i++)
			_boneWeights[i].clear();
		_boneWeights.clear();
	};

protected:
	
	virtual void Enable_VA() = 0;	
	virtual void Enable_VBO() = 0;	
	virtual void Disable_VA() = 0;	
	virtual void Disable_VBO() = 0;

protected:
	
	U32  		_VBOid;
	ptrdiff_t	_VBOoffsetPosition;
	ptrdiff_t	_VBOoffsetNormal;
	ptrdiff_t	_VBOoffsetTexcoord;
	ptrdiff_t	_VBOoffsetTangent;
	ptrdiff_t	_VBOoffsetBiTangent;

	
	std::vector<vec3>	_dataPosition;
	std::vector<vec3>	_dataNormal;
	std::vector<vec2>	_dataTexcoord;
	std::vector<vec3>	_dataTangent;
	std::vector<vec3>	_dataBiTangent;
	std::vector<std::vector<U8>> _boneIndices;
	std::vector<std::vector<U8>> _boneWeights;

	
};

#endif

