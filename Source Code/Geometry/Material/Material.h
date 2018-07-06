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

class Material : public Resource{

public:
  enum TextureUsage{
	  TEXTURE_BASE = 0,
	  TEXTURE_BUMP = 1,
	  TEXTURE_SECOND = 2
  };
typedef std::tr1::unordered_map<TextureUsage, Texture2D*> textureMap;

public:
  Material();
  Material(const Material& old);
  Material& operator=(const Material& old);
  ~Material();

  bool load(const std::string& name) {_name = name; return true;}
  bool unload();
  inline U8              getTextureCount() {return _textures.size();}
  Texture2D*	 const   getTexture(TextureUsage textureUsage);
  inline Shader* const   getShader()   {return _shader;}

  void setTexture(TextureUsage textureUsage, Texture2D* texture);
  void setShader(const std::string& shader);

  void setAmbient(const vec4& value) {_ambient = value; _materialMatrix.setCol(0,value);}
  void setDiffuse(const vec4& value) {_diffuse = value; _materialMatrix.setCol(1,value);}
  void setSpecular(const vec4& value) {_specular = value; _materialMatrix.setCol(2,value);}
  void setEmmisive(const vec4& value) {_emmissive = value; _materialMatrix.setCol(3,vec4(_shininess,value.x,value.y,value.z));}
  void setShininess(F32 value) {_shininess = value; _materialMatrix.setCol(3,vec4(value,_emmissive.x,_emmissive.y,_emmissive.z));}

  mat4& getMaterialMatrix() {return _materialMatrix;}

  void computeLightShaders(); //Set shaders;
  void skipComputeLightShaders();

private:
  vec4 _diffuse;           /* diffuse component */
  vec4 _ambient;           /* ambient component */
  vec4 _specular;          /* specular component */
  vec4 _emmissive;         /* emmissive component */
  F32 _shininess;          /* specular exponent */
  mat4    _materialMatrix; /* all properties bundled togheter */

  textureMap _textures;
  Shader* _shader;
  bool _computedLightShaders;
};

#endif