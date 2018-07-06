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

#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "core.h"
#include "Core/Resources/Headers/HardwareResource.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"

class Texture : public HardwareResource{
/*Abstract interface*/
public:
	virtual void Bind(U16 slot);
	virtual void Unbind(U16 slot);
	virtual void Destroy() = 0;
	virtual void LoadData(U32 target, U8* ptr, U16& w, U16& h, U8 d) = 0;
	virtual ~Texture() {}

protected:
	template<typename T>
	friend class ImplResourceLoader;
	virtual bool generateHWResource(const std::string& name) {return HardwareResource::generateHWResource(name);}
	virtual void Bind() const;
	virtual void Unbind() const;

public:
    bool LoadFile(U32 target, const std::string& name);
	void resize(U16 width, U16 height);

	inline U32 getTextureWrap(U32 index) {
		switch(index){
			default:
			case 0:	return _wrapU;
			case 1:	return _wrapV;
			case 2:	return _wrapW;
		};
	}

	inline I32 getFilter(U8 index){
		switch(index){
			case 0: return static_cast<I32>(_minFilter);
			case 1:	return static_cast<I32>(_magFilter);
		};
		return -1;
	}

	inline U32 getAnisotrophy()   const {return (U32)_maxAnisotrophy;}
	inline U32 getHandle()        const {return _handle;}
	inline U16 getWidth()         const {return _width;}
	inline U16 getHeight()        const {return _height;}
	inline U8  getBitDepth()      const {return _bitDepth;}
	inline bool isFlipped()       const {return _flipped;}
	inline bool hasTransparency() const {return _hasTransparency;}

	inline void enableGenerateMipmaps(const bool generateMipMaps)       {_generateMipmaps=generateMipMaps;}

	inline void  setAnisotrophyLevel(U8 anisoLevel)             {_maxAnisotrophy = anisoLevel;}
	inline void  setTextureWrap(U32 wrapU, U32 wrapV,U32 wrapW) {_wrapU = wrapU; _wrapV = wrapV; _wrapW = wrapW;}
	inline void  setTextureFilters(U8 minFilter, U8 magFilter)  {_minFilter = minFilter; _magFilter = magFilter;}

protected:
	Texture(const bool flipped = false);
	static bool checkBinding(U16 unit, U32 handle);

protected:
	boost::atomic<U32>	_handle;
	U16 _width,_height;
	U8  _bitDepth;
	U8  _numMipMaps;
	bool _flipped;
	bool _bound;
	bool _hasTransparency;
	bool _generateMipmaps;
	mat4<F32>  _transformMatrix;
	U32  _wrapU, _wrapV, _wrapW;
	U8 _minFilter,_magFilter;
	U8 _maxAnisotrophy;
	static Unordered_map<U8/*slot*/, U32/*textureHandle*/> textureBoundMap;
};

#endif
