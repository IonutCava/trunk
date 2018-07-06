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

class vertex 
{
public:
	vertex() : temp(new vec3(x,y,z)) {}
	vertex(F32 _x, F32 _y, F32 _z) : x(_x),y(_y),z(_z), temp(new vec3(_x,_y,_z)) {}
	vertex(vertex const &rhs): x(rhs.x), y(rhs.y), z(rhs.z) {}
   //Coordonatele X,Y,Z ale fiecarui vertex din obiectul OBJ
   //void operator=(vertex &p1){	x = p1.x; y = p1.y; z = p1.z; }
   void operator=(vertex const &p1) {x = p1.x; y = p1.y; z = p1.z;}
  
   /*binds the values of a vector's x,y,z to the vertex x,y,z*/
   void bind(vec3& v){   x = v.x; y = v.y; z = v.z;   }
   void normalize()
   { 
	    F32 l = sqrt(x*x + y*y + z*z);
		x /= l; 	y /= l;		z /= l;
   }
   F32 dot(vertex& v)
   {
	   return temp->dot(vec3(v.x,v.y,v.z));
   }
   bool equals(vertex& v,F32 epsilon)
   {
	   return temp->compare(vec3(v.x,v.y,v.z),epsilon);
   }
public:
	   F32 x,y,z;
private:
	  vec3* temp;
};

typedef vertex normal;
class color{public:	F32	r,g,b,u; };
class texCoord{	public:	F32	u,v; };
class tangent{	public:	F32 x, y, z, w;	};

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

//o fata simpla a unui model este compusa din 3 vertecsi
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
  Group*         next;           /* pointer to next group in model */
};

/* Material: Structure that defines a material in a model. 
 */
class Material
{
public:
  std::string name;            /* name of the material */
  F32 diffuse[4];           /* diffuse component */
  F32 ambient[4];           /* ambient component */
  F32 specular[4];          /* specular component */
  F32 emmissive[4];         /* emmissive component */
  F32 shininess;            /* specular exponent */
  U32 textureId;     //Pointer to Texture Object
};


class TextureInfo									
{
public:
	UBYTE	header[6];					// First 6 Useful Bytes From The Header
	U32	imageSize;					// Used To Store The Image Size When Setting Aside Ram
	UBYTE*  imageData;					// Image Data (Up To 32 Bits)
	UBYTE   bpp;					    // Image Color Depth In Bits Per Pixel
	short int       width, height;				// Image Width and Height
	U32	id;					    	// Texture ID Used To Select A Texture
	U32	mode;						// Image Type (GL_RGB, GL_RGBA)
	UBYTE   type;						// Color or Grayscale
	int             status;						// Image load status
	U32    temp;						// Temporary Variable (used as an index in a vector for example)
	string     name;						    // Texture name
	U32    bumptexID;					// ID bump map 
};

#endif