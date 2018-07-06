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

#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "core.h"
#include "Core/Resources/Headers/HardwareResource.h"

class Texture : public HardwareResource{

/*Abstract interface*/
public:
	virtual void Bind(U16 slot);
	virtual void Unbind(U16 slot);
	virtual void Destroy() = 0;
	virtual void LoadData(U32 target, U8* ptr, U16& w, U16& h, U8 d) = 0;
	virtual ~Texture() {D_PRINT_FN("Deleting texture  [ %s ]",getResourceLocation().c_str());}

	enum TextureFilters{
		LINEAR					= 0x0000,
		NEAREST					= 0x0001,
		NEAREST_MIPMAP_NEAREST  = 0x0002,
		LINEAR_MIPMAP_NEAREST   = 0x0003,
		NEAREST_MIPMAP_LINEAR   = 0x0004,
		LINEAR_MIPMAP_LINEAR    = 0x0005
	};
	enum TextureWrap {
		 /** A texture coordinate u|v is translated to u%1|v%1 
		 */
		TextureWrap_Wrap = 0x0,

		/** Texture coordinates outside [0...1]
		 *  are clamped to the nearest valid value.
		 */
		TextureWrap_Clamp = 0x1,

		/** If the texture coordinates for a pixel are outside [0...1]
		 *  the texture is not applied to that pixel
		 */
		TextureWrap_Decal = 0x3,

		TextureWrap_Repeat = 0x4,

		TextureWrap_PLACEHOLDER = 0x5
	};

protected:
	template<typename T>
	friend class ImplResourceLoader;
	virtual bool generateHWResource(const std::string& name) {return HardwareResource::generateHWResource(name);}
	virtual void Bind() const;
	virtual void Unbind() const;

public:

	void resize(U16 width, U16 height);

	inline U32 getTextureWrap(U32 index) {
		switch(index){
			default:
			case 0:
				return _wrapU;
			case 1:
				return _wrapV;
			case 2:
				return _wrapW;

		};
	}   

	inline	U32 getHandle() const {return _handle;} 
	inline	U16 getWidth() const {return _width;}
	inline	U16 getHeight() const {return _height;}
	inline	U8  getBitDepth() const {return _bitDepth;}
	inline	bool isFlipped()  {return _flipped;}
	inline  bool hasTransparency() {return _hasTransparency;}
	static void enableGenerateMipmaps(bool generateMipMaps) {_generateMipmaps=generateMipMaps;}

	bool LoadFile(U32 target, const std::string& name);

	inline 	void  setTextureWrap(TextureWrap wrapU, TextureWrap wrapV,TextureWrap wrapW){_wrapU = wrapU; _wrapV = wrapV; _wrapW = wrapW;}
	inline	void  setTextureFilters(U8 minFilter, U8 magFilter) {_minFilter = minFilter; _magFilter = magFilter;}
	
protected:
	Texture(bool flipped = false);
	static bool checkBinding(U16 unit, U32 handle);

protected:
	U32	_handle;
	U16 _width,_height;
	U8  _bitDepth;			
	bool _flipped;
	bool _bound;
	bool _hasTransparency;
	static bool _generateMipmaps;	
	mat4<F32>  _transformMatrix;
	U32  _wrapU, _wrapV, _wrapW;
	U8 _minFilter,_magFilter;
	static unordered_map<U8/*slot*/, U32/*textureHandle*/> textureBoundMap;
};


#endif

