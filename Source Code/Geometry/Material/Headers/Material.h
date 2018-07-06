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

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "resource.h"
#include "Core/Headers/BaseClasses.h"

class Shader;
class Texture;
typedef Texture Texture2D;
class RenderState;
class Material : public Resource{

public:
  enum TextureUsage{
	  TEXTURE_BASE = 0,
	  TEXTURE_BUMP = 1,
	  TEXTURE_NORMALMAP = 2,
	  TEXTURE_SECOND = 3,
	  TEXTURE_OPACITY = 4,
	  TEXTURE_SPECULAR = 5,
	  TEXTURE_DISPLACEMENT = 6,
	  TEXTURE_EMISSIVE = 7,
	  TEXTURE_AMBIENT = 8,
	  TEXTURE_SHININESS = 9,
	  TEXTURE_MIRROR = 10,
	  TEXTURE_LIGHTMAP = 11
  };
typedef unordered_map<TextureUsage, Texture2D*> textureMap;

public:
  Material();
  ~Material();

  bool load(const std::string& name) {_name = name; return true;}
  bool unload();
  inline U8              getTextureCount() {return _textures.size();}
  Texture2D*	 const   getTexture(TextureUsage textureUsage);
  Shader*        const   getShader();
  inline bool            isDirty() {return _dirty;}
  void setTexture(TextureUsage textureUsage, Texture2D* const texture);
  void setShader(const std::string& shader);
  void setTwoSided(bool state);
  bool isTwoSided() {return _twoSided;}
  RenderState& getRenderState() const {return *_state;}

  inline void setAmbient(const vec4& value) {_ambient = value; _materialMatrix.setCol(0,value);}
  inline void setDiffuse(const vec4& value) {_diffuse = value; _materialMatrix.setCol(1,value);}
  inline void setSpecular(const vec4& value) {_specular = value; _materialMatrix.setCol(2,value);}
  inline void setEmissive(const vec3& value) {_emissive = value; _materialMatrix.setCol(3,vec4(_shininess,value.x,value.y,value.z));}
  inline void setShininess(F32 value) {_shininess = value; _materialMatrix.setCol(3,vec4(value,_emissive.x,_emissive.y,_emissive.z));}
  inline void setCastsShadows(bool state) {_castsShadows = state;}
  inline void setReceivesShadows(bool state) {_receiveShadows = state;}

  inline mat4& getMaterialMatrix()  {return _materialMatrix;}
  inline I32   getMaterialId()      {return _matId;}
  inline bool  getCastsShadows()    {return _castsShadows;}
  inline bool  getReceivesShadows() {return _receiveShadows;}



  void computeLightShaders(); //Set shaders;
  void createCopy();
  void removeCopy();
  void dumpToXML();

private:
  vec4 _diffuse;           /* diffuse component */
  vec4 _ambient;           /* ambient component */
  vec4 _specular;          /* specular component */
  vec3 _emissive;          /* emissive component */
  F32 _shininess;          /* specular exponent */
  mat4 _materialMatrix; /* all properties bundled togheter */

  textureMap _textures;
  std::string _shader; /*shader name*/
  bool _computedLightShaders;
  I32  _matId;
  bool _dirty;
  bool _twoSided;
  bool _castsShadows;
  bool _receiveShadows;
  RenderState* _state;
};

#endif