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

#ifndef _TEXTURE_DESCRIPTOR_H_
#define _TEXTURE_DESCRIPTOR_H_

#include "core.h"

struct SamplerDescriptor {
    SamplerDescriptor()
    {
    }

    inline void setWrapMode(TextureWrap wrapU = TEXTURE_CLAMP, TextureWrap wrapV = TEXTURE_CLAMP, TextureWrap wrapW = TEXTURE_CLAMP){
		_wrapU = wrapU; _wrapV = wrapV; _wrapW = wrapW;
	}

	inline void setFilters(TextureFilter minFilter = TEXTURE_FILTER_LINEAR, TextureFilter magFilter = TEXTURE_FILTER_LINEAR){
		_minFilter = minFilter; _magFilter = magFilter;
	}

	inline void setAlignment(U8 packAlignment = 1, U8 unpackAlignment = 1) {
		_packAlignment = packAlignment; _unpackAlignment = unpackAlignment;
	}

	inline void setMipLevels(U16 mipMinLevel = 0, U16 mipMaxLevel = 1000) {
		_mipMinLevel = mipMinLevel; _mipMaxLevel = mipMaxLevel;
	}

	inline void toggleMipMaps(bool state) {_generateMipMaps = state;}
	inline void setAnisotrophy(U8 value) {_anisotrophyLevel = value;}

    TextureFilter  _minFilter;
	TextureFilter  _magFilter;
	TextureWrap    _wrapU;
	TextureWrap    _wrapV;
	TextureWrap    _wrapW;
    bool           _generateMipMaps; ///<create automatic MipMaps
	bool           _useRefCompare;   ///<use red channel as comparison (e.g. for shadows)
	U8             _anisotrophyLevel;
	U8             _packAlignment;
	U8             _unpackAlignment;
	U16            _mipMinLevel;
	U16            _mipMaxLevel;

	ComparisonFunction _cmpFunc; ///<Used by RefCompare
};

///Use to define a texture with details such as type, image formats, etc
struct TextureDescriptor {
	enum AttachmentType{
		Color0 = 0,
		Color1,
		Color2,
		Color3,
		Depth
	};

	TextureDescriptor() : _type(TextureType_PLACEHOLDER),
						  _format(IMAGE_FORMAT_PLACEHOLDER),
						  _internalFormat(IMAGE_FORMAT_PLACEHOLDER),
						  _dataType(GDF_PLACEHOLDER),
						  _generateMipMaps(true),
						  _useRefCompare(false),
						  _anisotrophyLevel(0)
	{
		setDefaultValues();
	}

	TextureDescriptor(TextureType type,
				      GFXImageFormat format,
					  GFXImageFormat internalFormat,
					  GFXDataFormat dataType) : _type(type),
											    _format(format),
											    _internalFormat(internalFormat),
												_dataType(dataType),
												_generateMipMaps(true),
												_useRefCompare(false),
												_anisotrophyLevel(0)
	{
		setDefaultValues();
	}

	inline void setDefaultValues(){
		setWrapMode();
		setFilters();
		setAlignment();
		setMipLevels();
		_cmpFunc = CMP_FUNC_LEQUAL;
	}

	inline void setWrapMode(TextureWrap wrapU = TEXTURE_CLAMP, TextureWrap wrapV = TEXTURE_CLAMP, TextureWrap wrapW = TEXTURE_CLAMP){
		_wrapU = wrapU; _wrapV = wrapV; _wrapW = wrapW;
	}

	inline void setFilters(TextureFilter minFilter = TEXTURE_FILTER_LINEAR, TextureFilter magFilter = TEXTURE_FILTER_LINEAR){
		_minFilter = minFilter; _magFilter = magFilter;
	}

	inline void setAlignment(U8 packAlignment = 1, U8 unpackAlignment = 1) {
		_packAlignment = packAlignment; _unpackAlignment = unpackAlignment;
	}

	inline void setMipLevels(U16 mipMinLevel = 0, U16 mipMaxLevel = 1000) {
		_mipMinLevel = mipMinLevel; _mipMaxLevel = mipMaxLevel;
	}

	inline void toggleMipMaps(bool state) {_generateMipMaps = state;}
	inline void setAnisotrophy(U8 value) {_anisotrophyLevel = value;}

	GFXImageFormat _format;
	GFXImageFormat _internalFormat;
	GFXImageFormat _depthCompareMode;
	GFXDataFormat  _dataType;
	TextureType    _type;
	TextureFilter  _minFilter;
	TextureFilter  _magFilter;
	TextureWrap    _wrapU;
	TextureWrap    _wrapV;
	TextureWrap    _wrapW;
	bool           _generateMipMaps; ///<create automatic MipMaps
	bool           _useRefCompare;   ///<use red channel as comparison (e.g. for shadows)
	U8             _anisotrophyLevel;
	U8             _packAlignment;
	U8             _unpackAlignment;
	U16            _mipMinLevel;
	U16            _mipMaxLevel;

	ComparisonFunction _cmpFunc; ///<Used by RefCompare
};

#endif