/*
   Copyright (c) 2013 DIVIDE-Studio
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