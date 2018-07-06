#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "resource.h"
class Texture;
typedef Texture Texture2D;
class Material
{
public:
  Material();
  Material(const Material& old);
  Material& operator=(const Material& old);
  ~Material();

  std::string name;       /* name of the material */
  vec4 diffuse;           /* diffuse component */
  vec4 ambient;           /* ambient component */
  vec4 specular;          /* specular component */
  vec4 emmissive;         /* emmissive component */
  F32 shininess;          /* specular exponent */
  std::vector<Texture2D*> textures;
  Texture2D* bumpMap;
};

#endif