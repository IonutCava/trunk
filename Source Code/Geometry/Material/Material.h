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
#include "Utility/Headers/BaseClasses.h"
class Shader;
class Texture;
typedef Texture Texture2D;
class RenderState;
class Material : public Resource{

public:
  enum TextureUsage{
	  TEXTURE_BASE = 0,
	  TEXTURE_BUMP = 1,
	  TEXTURE_SECOND = 2
  };
typedef unordered_map<TextureUsage, Texture2D*> textureMap;

public:
  Material();
  ~Material();

  bool load(const std::string& name) {_name = name; return true;}
  bool unload();
  inline U8              getTextureCount() {return _textures.size();}
  Texture2D*	 const   getTexture(TextureUsage textureUsage);
  inline Shader* const   getShader()   {return _shader;}
  inline bool            isDirty() {return _dirty;}
  void setTexture(TextureUsage textureUsage, Texture2D* texture);
  void setShader(const std::string& shader);
  void setTwoSided(bool state);
  RenderState& getRenderState() const {return *_state;}
  void setAmbient(const vec4& value) {_ambient = value; _materialMatrix.setCol(0,value);}
  void setDiffuse(const vec4& value) {_diffuse = value; _materialMatrix.setCol(1,value);}
  void setSpecular(const vec4& value) {_specular = value; _materialMatrix.setCol(2,value);}
  void setEmissive(const vec3& value) {_emissive = value; _materialMatrix.setCol(3,vec4(_shininess,value.x,value.y,value.z));}
  void setShininess(F32 value) {_shininess = value; _materialMatrix.setCol(3,vec4(value,_emissive.x,_emissive.y,_emissive.z));}

  inline mat4& getMaterialMatrix() {return _materialMatrix;}
  inline U8    getMaterialId()     {return _matId;}
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
  Shader* _shader;
  bool _computedLightShaders;
  U8   _matId;
  bool _dirty;
  bool _twoSided;
  RenderState* _state;
};

#endif