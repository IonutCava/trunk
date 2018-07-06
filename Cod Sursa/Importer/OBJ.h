/*    
      glm.h
      Nate Robins, 1997, 2000
      nate@pobox.com, http://www.pobox.com/~nate
 
      Wavefront OBJ model file format reader/writer/manipulator.

      Includes routines for generating smooth normals with
      preservation of edges, welding redundant vertices & texture
      coordinate generation (spheremap and planar projections) + more.

      Improved version:
	  Tudor Carean - April 2008 - added texture support

 */
#ifndef GLM_H_
#define GLM_H_
#include "resource.h"
#include "Utility/Headers/MathClasses.h"
#include "Utility/Headers/DataTypes.h"
#include "Utility/Headers/BaseClasses.h"

using namespace std;

#define GLM_NONE     (0)            /* render with only vertices */
#define GLM_FLAT     (1 << 0)       /* render with facet normals */
#define GLM_SMOOTH   (1 << 1)       /* render with vertex normals */
#define GLM_TEXTURE  (1 << 2)       /* render with texture coords */
#define GLM_COLOR    (1 << 3)       /* render with colors */
#define GLM_MATERIAL (1 << 4)       /* render with materials */
#define GLM_2_SIDED  (1 << 5)       /* AVL Draw two sided normals */

static int GLM_BLENDING;   //AVL Blending flag 

enum MeshType
{
	MESH_CONVEX,
	MESH_TRIANGLE
};

/* ImportedModel: Structure that defines a model.
 */
class Shader;
class SubMesh;
class ImportedModel : public Resource {
public:
	~ImportedModel(){unload();}
	ImportedModel(){}
	bool load(const string &name) {return true;}
	bool unload();
  char*    pathname;            /* path to this model */
  char*    mtllibname;          /* name of the material library */

  vector<vertex> vertices;      /* array of vertices  */
  vector<normal> normals;       /* array of normals */
  vector<texCoord> texcoords;   /* array of texture coordinates */
  vector<vertex> facetnorms;          /* array of facetnorms */
  vector<triangle> triangles;          /* array of triangles */ 
  vector<simpletriangle> SimpleFaceArray; /*array of simple triangles*/
  vector<Material>    materials;       /* array of materials */
  vector<Group>       groups;          /* linked list of groups */ 
  vector<TextureInfo>  textures;
  vector<TextureInfo>::iterator TextureIterator;
  F32 ScaleFactor;
  vec3 rotation;                      /*(X degrees, Y degrees, Z degrees)*/
  vec3 position;                      /* position of the model */
  string ModelName;
  string NormalMapName;
  int IdNormalMap;
  bool vegetation;
  Shader* shader;
  U32 ListID;
};

class mycallback
{
public:
	void (*loadcallback)(F32,char *);
	int start; 
	int end;
	char *text;
};

void glmDraw(ImportedModel* model, U32 mode,char *drawonly, U32 ListID);

F32 glmDot(F32* u, F32* v);
void  glmFirstPass(ImportedModel* model, FILE* file); 
void  glmSecondPass(ImportedModel* model, FILE* file);
F32 glmUnitize(ImportedModel* model); //redimensioneaza modelul si il muta in origine
void  glmDimensions(ImportedModel* model, F32* dimensions);
void  glmScale(ImportedModel* model, F32 scale);
void  glmReverseWinding(ImportedModel* model);
void  glmFacetNormals(ImportedModel* model);
void  glmSetScale(ImportedModel* model,F32 ScaleFactor);
void  glmSetPosition(ImportedModel* model,vec3 position);
void  glmSetRotation(ImportedModel* model,vec3 rotation);
void  glmSetNormalMap(ImportedModel* model, std::string NormalMap);
static GLuint glmFindOrAddTexture(ImportedModel* model, std::string name);
/* glmVertexNormals: Generates smooth vertex normals for a model.
 * First builds a list of all the triangles each vertex is in.  Then
 * loops through each vertex in the the list averaging all the facet
 * normals of the triangles each vertex is in.  Finally, sets the
 * normal index in the triangle for the vertex to the generated smooth
 * normal.  If the dot product of a facet normal and the facet normal
 * associated with the first triangle in the list of triangles the
 * current vertex is in is greater than the cosine of the angle
 * parameter to the function, that facet normal is not added into the
 * average normal calculation and the corresponding vertex is given
 * the facet normal.  This tends to preserve hard edges.  The angle to
 * use depends on the model, but 90 degrees is usually a good start.
 *
 * model - initialized ImportedModel structure
 * angle - maximum angle (in degrees) to smooth across
 */
void
glmVertexNormals(ImportedModel* model, F32 angle);

/* glmLinearTexture: Generates texture coordinates according to a
 * linear projection of the texture map.  It generates these by
 * linearly mapping the vertices onto a square.
 *
 * model - pointer to initialized ImportedModel structure
 */
void
glmLinearTexture(ImportedModel* model);

/* glmSpheremapTexture: Generates texture coordinates according to a
 * spherical projection of the texture map.  Sometimes referred to as
 * spheremap, or reflection map texture coordinates.  It generates
 * these by using the normal to calculate where that vertex would map
 * onto a sphere.  Since it is impossible to map something flat
 * perfectly onto something spherical, there is distortion at the
 * poles.  This particular implementation causes the poles along the X
 * axis to be distorted.
 *
 * model - pointer to initialized ImportedModel structure
 */
void
glmSpheremapTexture(ImportedModel* model);

/* glmDelete: Deletes a ImportedModel structure.
 *
 * model - initialized ImportedModel structure
 */
void
glmDelete(ImportedModel* model, bool keepTextures);



/* glmWriteOBJ: Writes a model description in Wavefront .OBJ format to
 * a file.
 *
 * model    - initialized ImportedModel structure
 * filename - name of the file to write the Wavefront .OBJ format data to
 * mode     - a bitwise or of values describing what is written to the file
 *            GLM_NONE    -  write only vertices
 *            GLM_FLAT    -  write facet normals
 *            GLM_SMOOTH  -  write vertex normals
 *            GLM_TEXTURE -  write texture coords
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
void
glmWriteOBJ(ImportedModel* model, char* filename, U32 mode);

/* glmDraw: Renders the model to the current OpenGL context using the
 * mode specified.
 *
 * model    - initialized ImportedModel structure
 * mode     - a bitwise OR of values describing what is to be rendered.
 *            GLM_NONE    -  render with only vertices
 *            GLM_FLAT    -  render with facet normals
 *            GLM_SMOOTH  -  render with vertex normals
 *            GLM_TEXTURE -  render with texture coords
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
void
glmDraw(ImportedModel* model, U32 mode, U32 ListID);

/* glmList: Generates and returns a display list for the model using
 * the mode specified.
 *
 * model    - initialized ImportedModel structure
 * mode     - a bitwise OR of values describing what is to be rendered.
 *            GLM_NONE    -  render with only vertices
 *            GLM_FLAT    -  render with facet normals
 *            GLM_SMOOTH  -  render with vertex normals
 *            GLM_TEXTURE -  render with texture coords
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.  
 */
U32
glmList(ImportedModel* model, U32 mode);

/* glmWeld: eliminate (weld) vectors that are within an epsilon of
 * each other.
 *
 * model      - initialized ImportedModel structure
 * epsilon    - maximum difference between vertices
 *              ( 0.00001 is a good start for a unitized model)
 *
 */
void
glmWeld(ImportedModel* model, F32 epsilon);

void 
CreateSimpleFaceArray(ImportedModel *pMesh);
/* glmReadPPM: read a PPM raw (type P6) file.  The PPM file has a header
 * that should look something like:
 *
 *    P6
 *    # comment
 *    width height max_value
 *    rgbrgbrgb...
 *
 * where "P6" is the magic cookie which identifies the file type and
 * should be the only characters on the first line followed by a
 * carriage return.  Any line starting with a # mark will be treated
 * as a comment and discarded.   After the magic cookie, three integer
 * values are expected: width, height of the image and the maximum
 * value for a pixel (max_value must be < 256 for PPM raw files).  The
 * data section consists of width*height rgb triplets (one byte each)
 * in binary format (i.e., such as that written with fwrite() or
 * equivalent).
 *
 * The rgb data is returned as an array of unsigned chars (packed
 * rgb).  The malloc()'d memory should be free()'d by the caller.  If
 * an error occurs, an error message is sent to stderr and NULL is
 * returned.
 *
 * filename   - name of the .ppm file.
 * width      - will contain the width of the image on return.
 * height     - will contain the height of the image on return.
 *
 */
UBYTE* 
glmReadPPM(char* filename, int* width, int* height);
UBYTE* 
glmReadTGA(char* filename, int* width, int* height);

Group*
glmFindGroup(ImportedModel* model, const string& name);
//AVL Prototypes
//AVL Flip Texture
GLvoid glmFlipTexture(UBYTE* texture, int width, int height);

//AVL Flip Model Textures
GLvoid glmFlipModelTextures(ImportedModel* model);
#endif