#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "resource.h"
class Shader;
class Texture;
typedef Texture Texture2D;
class Material
{
public:
  enum TextureUsage{
	  TEXTURE_BASE = 0,
	  TEXTURE_BUMP = 1,
	  TEXTURE_SECOND = 2
  };

public:
  Material();
  Material(const Material& old);
  Material& operator=(const Material& old);
  ~Material();

  inline U8              getTextureCount() {return _textures.size();}
  Texture2D*	 const   getTexture(TextureUsage textureUsage);
  inline Shader* const   getShader()   {return _shader;}

  void setTexture(TextureUsage textureUsage, Texture2D* texture);
  void setShader(Shader* shader);

  void clear();

public:
  std::string name;       /* name of the material */
  vec4 diffuse;           /* diffuse component */
  vec4 ambient;           /* ambient component */
  vec4 specular;          /* specular component */
  vec4 emmissive;         /* emmissive component */
  F32 shininess;          /* specular exponent */
  
private:
  typedef std::tr1::unordered_map<TextureUsage, Texture2D*> textureMap;
  textureMap _textures;
  Shader* _shader;
};

#endif