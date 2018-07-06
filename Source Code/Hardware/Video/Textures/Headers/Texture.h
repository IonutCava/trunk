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
