#define U8  unsigned char
#define U16 unsigned short
#define U32 unsigned int
#define U64 unsigned long long

#define I8  signed char
#define I16 signed short
#define I32 signed int
#define I64 signed long long

#define F32 float
#define D32 double

#ifndef DATA_TYPES_H
#define DATA_TYPES_H
#include "Utility/Headers/MathClasses.h"
#include <string>
using namespace std;
#define COORD(x,y,w)	((y)*(w)+(x))


//o fata simpla a unui model este compusa din indicii a 3 vertecsi
//used for PhysX
class simpletriangle{public:   U32 Vertex[3]; };



/* Material: Structure that defines a material in a model. 
 */
class Texture;
typedef Texture Texture2D;
typedef Texture TextureCubemap;

class Material
{
public:
  string name;              /* name of the material */
  F32 diffuse[4];           /* diffuse component */
  F32 ambient[4];           /* ambient component */
  F32 specular[4];          /* specular component */
  F32 emmissive[4];         /* emmissive component */
  F32 shininess;            /* specular exponent */
  U32 textureId;            /* Pointer to Texture Object */
  Texture2D* texture;
};

#endif



