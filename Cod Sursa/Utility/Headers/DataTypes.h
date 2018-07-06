#define U32 unsigned int
#define F32 float
#define D32 double
#define UBYTE unsigned char

#ifndef DATA_TYPES_H
#define DATA_TYPES_H
#include "Utility/Headers/MathClasses.h"
#include <string>
using namespace std;
#define COORD(x,y,w)	((y)*(w)+(x))

//A Vertex or Normal is a 3-compononent vector
//As a naming convention vec3 is used for vector calculations (ray intersection, ligh direction etc) and 
//						 vertex is used for geomentry data storage
typedef vec3 normal;
typedef vec3 vertex;

class color   {public:	F32	r,g,b,u; };
class texCoord{	public:	F32	u,v; };
class tangent {	public:	F32 x, y, z, w;	};

class triangle {
public:
  U32 vindices[3];           /* array of triangle vertex indices */
  U32 nindices[3];           /* array of triangle normal indices */
  U32 tindices[3];           /* array of triangle texcoord indices*/
  U32 findex;                /* index of triangle facet normal */
  U32 vecini[3];
  U32 material;
  bool visible;
};

//o fata simpla a unui model este compusa din indicii a 3 vertecsi
//used for PhysX
class simpletriangle{public:   U32 Vertex[3]; };

//OBJ specific
class Node {
public:
    U32 index;
    bool      averaged;
    Node* next;
};

class Group {
public:
  char*          name;           /* name of this group */
  U32            numtriangles;   /* number of triangles in this group */
  U32*           triangles;      /* array of triangle indices */
  U32            material;       /* index to material for group */
};

/* Material: Structure that defines a material in a model. 
 */
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
};

#endif



