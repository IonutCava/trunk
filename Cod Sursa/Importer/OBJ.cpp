/*    
      glm.c
      Nate Robins, 1997, 2000
      nate@pobox.com, http://www.pobox.com/~nate
 
      Wavefront OBJ model file format reader/writer/manipulator.

      Includes routines for generating smooth normals with
      preservation of edges, welding redundant vertices & texture
      coordinate generation (spheremap and planar projections) + more.
  
      changes by F. Devernay:
      - warning/error functions in glm_util.c
      - added glmStrStrip function to handle filenames with spaces
      - handle material-by-face if there is more than one usemtl in a group
      - removed "static" from glmDraw variables (so that the code is reentrant)
      - write obj with material-by-face
      - added blending support, from GLM_AVL http://www.avl.iu.edu/projects/GLM_AVL/

      TODO:
      - allow CR, CRLF, or LF as end-of-line in input obj and mtl
      - glmVertexNormals(): have an option to add normals only where undefined
*/

#include "OBJ.h"
#include "Managers/TextureManager.h"
#include "Managers/ResourceManager.h"
#include "Hardware/Video/GFXDevice.h"

#define GLM_MAX_SHININESS 100.0
#define total_textures 5
#define MATERIAL_BY_FACE
#define T(x) (model->triangles[(x)])


/* strip leading and trailing whitespace from a string and return a newly
   allocated string containing the result (or NULL if the string is only 
   whitespace)*/

bool ImportedModel::unload()
{
	textures.clear(); 
	delete shader;
	groups.clear();
	materials.clear();
	SimpleFaceArray.clear();
	triangles.clear();
	facetnorms.clear();
	texcoords.clear();
	normals.clear();
	vertices.clear();
	delete[] mtllibname;
	delete[] pathname;
	
	return true;
}

void stripSpaces( string& str)
{
    size_t startpos = str.find_first_not_of(" \t");
    size_t endpos = str.find_last_not_of(" \t");

	if(( string::npos == startpos ) || ( string::npos == endpos))
	   str = "";
	else
	   str = str.substr( startpos, endpos-startpos+1 );

}
char *
__glmStrStrip(const char *s)
{
    int first;
    int last = strlen(s)-1;
    int len;
    int i;
    char * rets;

    i=0;
    while(i <= last &&
          (s[i]==' ' || s[i]=='\t' || s[i]=='\n' || s[i]=='\r'))
        i++;
    if (i>last)
        return NULL;
    first = i;
    i = last;
    while(i > first &&
          (s[i]==' ' || s[i]=='\t' || s[i]=='\n' || s[i]=='\r'))
        i--;
    last = i;
    len = last-first+1;
    rets = New char[len+1]; /* add a trailing 0 */
    memcpy(rets, s + first, len);
    rets[len] = 0;
    return rets;
}


/* glmWeldVectors: eliminate (weld) vectors that are within an
 * epsilon of each other.
 *
 * vectors     - array of GLfloat[3]'s to be welded
 * numvectors - number of GLfloat[3]'s in vectors
 * epsilon     - maximum difference between vectors 
 *
 */
static vertex*
glmWeldVectors(vertex* vectors, U32* numvectors, float epsilon)
{
    vertex* copies;
    U32   copied;
    U32   i, j;
    
    copies = New vertex[*numvectors + 1];
    memcpy(copies, vectors, (sizeof(vertex) * (*numvectors + 1)));
    
    copied = 1;
    for (i = 1; i <= *numvectors; i++) {
        for (j = 1; j <= copied; j++) {
            if (vectors[3 * i].equals(copies[3 * j], epsilon)) {
                goto duplicate;
            }
        }
        
        /* must not be any duplicates -- add to the copies array */
        copies[copied] = vectors[i];
        j = copied;             /* pass this along for below */
        copied++;
        
duplicate:
/* set the first component of this vector to point at the correct
        index into the new copies array */
        vectors[i].x = (F32)j;
    }
    
    *numvectors = copied-1;
    return copies;
}

void glmSetScale(ImportedModel* model,F32 ScaleFactor)
{
	model->ScaleFactor = ScaleFactor;
}

void glmSetNormalMap(ImportedModel* model, string NormalMap)
{
	model->NormalMapName = NormalMap.c_str();
	F32 width = 0, height = 0;
	if(model->NormalMapName.compare("not_used") != 0)
		model->IdNormalMap = TEXMANAGER.LoadDetailedTexture(model->NormalMapName, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,&width,&height);
	else 
		model->IdNormalMap = -1;
}

void glmSetRotation(ImportedModel* model,vec3 rotation)
{
	model->rotation = rotation;
}
void glmSetPosition(ImportedModel* model,vec3 position)
{
	model->position = position;
}

/* glmFindGroup: Find a group in the model */
static Group*
glmFindGroup(ImportedModel* model, const string& name)
{
    vector<Group>::iterator groupIterator;
	for(groupIterator = model->groups.begin(); groupIterator != model->groups.end(); groupIterator++)
        if (name.compare((*groupIterator).name) == 0)  return &(*groupIterator);
    return NULL;
}

/* glmAddGroup: Add a group to the model */
static Group*
glmAddGroup(ImportedModel* model, const string& name)
{
    Group* group;
    
    group = glmFindGroup(model, name);
    if (!group) {
        group = New Group;
		group->name = _strdup(name.c_str());
        group->material = 0;
        group->numtriangles = 0;
        group->triangles = NULL;
        group->next = &(*model->groups.end());
        model->groups.push_back(*group);
    }
    
    return group;
}

/* glmFindGroup: Find a material in the model */
static GLuint
glmFindMaterial(ImportedModel* model, std::string name)
{
    GLuint i;

    /* XXX doing a linear search on a string key'd list is pretty lame,
       but it works and is fast enough for now. */
    for (i = 0; i < model->materials.size(); i++) {
    if (model->materials[i].name.compare(name) == 0)
           goto found;
    }
    
    /* didn't find the name, so print a warning and return the default
    material (0). */
    cout << "glmFindMaterial():  can't find material " <<  name << endl;
    i = 0;
    
found:
    return i;
}

/* glmFindTexture: Find a texture in the model */
static GLuint
glmFindOrAddTexture(ImportedModel* model, std::string name)
{
    F32 width, height;
	
	for (model->TextureIterator = model->textures.begin(); model->TextureIterator != model->textures.end(); model->TextureIterator++) {
        if ((*model->TextureIterator).name.compare(name) == 0)
            return (*model->TextureIterator).temp;
    }

	ResourceManager& res = ResourceManager::getInstance();
	TextureInfo temp;
    temp.name = name;
	temp.temp = model->textures.size()-1;
	temp.id = TEXMANAGER.LoadDetailedTexture(name, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, &width, &height);
	temp.width = (short)width;
    temp.height = (short)height;
    cout << "Allocated texture " << model->textures.size()-1 << "(id=" << temp.id << ",width= " <<  width << ",height=" << height << endl;
	model->textures.push_back(temp);

    return model->textures.size()-1;
}

/* glmReadMTL: read a wavefront material library file
 *
 * model - properly initialized ImportedModel structure
 * name  - name of the material library
 */
static GLvoid
glmReadMTL(ImportedModel* model, std::string name)
{
    FILE* file;
    
	std::string filename;
    char* t_filename;
    char    buf[128];
    GLuint nummaterials, i;
    
    filename = "assets/modele/" + name;

    
    file = fopen(filename.c_str(), "r");
    if (!file) {
        cout << "glmReadMTL() failed: can't open material file "<< filename << endl;
			 
    }
    filename.empty();
    
    /* count the number of materials in the file */
    nummaterials = 1;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'n':               /* newmtl */
	    if(strncmp(buf, "newmtl", 6) != 0)
		cout << "glmReadMTL: Got " <<  buf << " instead of \"newmtl\" in file " << filename << endl;
            fgets(buf, sizeof(buf), file);
            nummaterials++;
            sscanf(buf, "%s %s", buf, buf);
            break;
        default:
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        }
    }
    
    rewind(file);
    Material mat;
    /* set the default material */
    for (i = 0; i < nummaterials; i++) {
        mat.shininess = 65.0f;
        mat.diffuse[0] = 0.8f;
        mat.diffuse[1] = 0.8f;
        mat.diffuse[2] = 0.8f;
        mat.diffuse[3] = 1.0f;
        mat.ambient[0] = 0.2f;
        mat.ambient[1] = 0.2f;
        mat.ambient[2] = 0.2f;
        mat.ambient[3] = 1.0f;
        mat.specular[0] = 0.0f;
        mat.specular[1] = 0.0f;
        mat.specular[2] = 0.0f;
        mat.specular[3] = 1.0f;
		mat.textureId = -1;
		model->materials.push_back(mat);
    }
    model->materials[0].name = _strdup("default");
    
    /* now, read in the data */
    nummaterials = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'n':               /* newmtl */

			fgets(buf, sizeof(buf), file);
			sscanf(buf, "%s %s", buf, buf);
			nummaterials++;
			model->materials[nummaterials].name = _strdup(buf);
			cout << "Added material: " << buf << endl;
            break;
        case 'N':
            switch(buf[1]) {
            case 's':
                fscanf(file, "%f", &model->materials[nummaterials].shininess);
                /* wavefront shininess is from [0, 1000], so scale for OpenGL */
                model->materials[nummaterials].shininess /= GLM_MAX_SHININESS;
                model->materials[nummaterials].shininess *= 128.0;
                break;
            case 'i':
                /* Refraction index.  Values range from 1 upwards. A value
                   of 1 will cause no refraction. A higher value implies
                   refraction. */
                fgets(buf, sizeof(buf), file);
                break;
            default:
                fgets(buf, sizeof(buf), file);
                break;
            }
	    break;
        case 'K':
            switch(buf[1]) {
            case 'd':
                fscanf(file, "%f %f %f",
		        &model->materials[nummaterials].diffuse[0],
		        &model->materials[nummaterials].diffuse[1],
		        &model->materials[nummaterials].diffuse[2]);
                break;
            case 's':
                fscanf(file, "%f %f %f",
		       &model->materials[nummaterials].specular[0],
		       &model->materials[nummaterials].specular[1],
		       &model->materials[nummaterials].specular[2]);
                break;
            case 'a':
                fscanf(file, "%f %f %f",
		       &model->materials[nummaterials].ambient[0],
		       &model->materials[nummaterials].ambient[1],
		       &model->materials[nummaterials].ambient[2]);
                break;
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
            }
            break;
        case 'd':
            /* d = Dissolve factor (pseudo-transparency).
               Values are from 0-1. 0 is completely transparent, 1 is opaque. */
		{
			F32 alpha;
			fscanf(file, "%f", &alpha);
			model->materials[nummaterials].diffuse[3] = alpha;
		}
		break;
		case 'i':
		
            /* illum = (0, 1, or 2) 0 to disable lighting, 1 for
               ambient & diffuse only (specular color set to black), 2
               for full lighting. I've also seen values of 3 and 4 for
               'illum'... when there's a 3 there, there's often a
               'sharpness' attribute, but I didn't find any
               explanation. And I think the 4 illum value is supposed
               to denote two-sided polygons, but I kinda get the
               impression that some people just make stuff up and add
               whatever they want to these files, so there could be
               anything in there ;). */
				{
					int illum;
					fscanf(file, "%d", &illum);
				}
		break;
        case 'm':
            /* texture map */
            filename = New char[FILENAME_MAX];
			fgets((char*)filename.c_str(), FILENAME_MAX, file);
			//stripSpaces(filename);
            t_filename = __glmStrStrip((char*)filename.c_str());
            filename.empty();
            if(strncmp(buf, "map_Kd", 6) == 0) {
				model->materials[nummaterials].textureId = glmFindOrAddTexture(model, std::string(t_filename));
				free(t_filename);
			}else{
                free(t_filename);
				fgets(buf, sizeof(buf), file);
            }
            break;
        case 'r':
            /* reflection type and filename (?) */
			fgets(buf, sizeof(buf), file);
            cout << "reflection type ignored: r" << buf << endl;
	    break;
	default:
	    /* eat up rest of line */
	    fgets(buf, sizeof(buf), file);
	    break;
        }
    }
    fclose(file);
}

/* glmFirstPass: first pass at a Wavefront OBJ file that gets all the
 * statistics of the model (such as #vertices, #normals, etc)
 *
 * model - properly initialized ImportedModel structure
 * file  - (fopen'd) file descriptor 
 */

void glmFirstPass(ImportedModel* model, FILE* file) 
{
    Group*  group;               /* current group */
    unsigned    v, n, t;
    char        buf[128];
    string      strTemp;
    /* make a default group */
    group = glmAddGroup(model, string("default"));
    

    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
       	case 'm':
			if(strncmp(buf, "mtllib", 6) != 0)
				cout << "glmReadOBJ: Got "<< buf << " instead of \"mtllib\"" << endl;
			fgets(buf, sizeof(buf), file);
			sscanf(buf, "%s %s", buf, buf);
			strTemp = ((char*)buf);
			//stripSpaces(strTemp);
			model->mtllibname = __glmStrStrip((char*)strTemp.c_str());
			glmReadMTL(model, model->mtllibname);
			break;
		case 'u':
			if(strncmp(buf, "usemtl", 6) != 0)
			cout << "glmReadOBJ: Got " << buf << " instead of \"usemtl\"" << endl;
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		case 'g':               /* group */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
#if SINGLE_STRING_GROUP_NAMES
			sscanf(buf, "%s", buf);
#else
			buf[strlen(buf)-1] = '\0';  /* nuke '\n' */
#endif
			group = glmAddGroup(model, string(buf));
			break;
		case 'f':               /* face */
			v = n = t = 0;
			fscanf(file, "%s", buf);
			/* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
            if (strstr(buf, "//")) {
                    /* v//n */
                    sscanf(buf, "%d//%d", &v, &n);
                    fscanf(file, "%d//%d", &v, &n);
                    fscanf(file, "%d//%d", &v, &n);
                    group->numtriangles++;
                    while(fscanf(file, "%d//%d", &v, &n) > 0) {
                        group->numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
                    /* v/t/n */
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    group->numtriangles++;
                    while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
                        group->numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
                    /* v/t */
                    fscanf(file, "%d/%d", &v, &t);
                    fscanf(file, "%d/%d", &v, &t);
                    group->numtriangles++;
                    while(fscanf(file, "%d/%d", &v, &t) > 0) {
                        group->numtriangles++;
                    }
                } else {
                    /* v */
                    fscanf(file, "%d", &v);
                    fscanf(file, "%d", &v);
                    group->numtriangles++;
                    while(fscanf(file, "%d", &v) > 0) {
                        group->numtriangles++;
                    }
                }
                break;
                
		default:
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
        }
  }
  
}

void glmSecondPass(ImportedModel* model, FILE* file) 
{
    GLuint  numtriangles;       /* number of triangles in model */
    Group* group;            /* current group pointer */
    GLuint  material;           /* current material */
    GLuint  v, n, t;
    char        buf[128];
    
       /* on the second pass through the file, read all the data into the
    allocated arrays */
    numtriangles = 0;
    material = 0;
	vertex tempV;
	normal tempN;
	texCoord tempT;

    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'v':               /* v, vn, vt */
            switch(buf[1]) {
            case '\0':          /* vertex */
                fscanf(file, "%f %f %f", 
                    &tempV.x, 
                    &tempV.y, 
                    &tempV.z);
				model->vertices.push_back(tempV);
                break;
            case 'n':           /* normal */
                fscanf(file, "%f %f %f", 
                    &tempN.x,
                    &tempN.y, 
                    &tempN.z);
                model->normals.push_back(tempN);
                break;
            case 't':           /* texcoord */
                fscanf(file, "%f %f", 
                    &tempT.u,
                    &tempT.v);
				model->texcoords.push_back(tempT);
                break;
            }
            break;
            case 'u':
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                material = glmFindMaterial(model, buf);
                if(!group->material && group->numtriangles)
                    group->material = material;
                break;
            case 'g':               /* group */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
#if SINGLE_STRING_GROUP_NAMES
                sscanf(buf, "%s", buf);
#else
                buf[strlen(buf)-1] = '\0';  /* nuke '\n' */
#endif
                group = glmFindGroup(model, string(buf));
#ifndef MATERIAL_BY_FACE
                group->material = material;
#endif
                break;
            case 'f':               /* face */
                v = n = t = 0;
		T(numtriangles).findex = -1;
                if(group->material == 0)
                    group->material = material;
                T(numtriangles).material = material;
                fscanf(file, "%s", buf);
                /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
                if (strstr(buf, "//")) {
                    /* v//n */
                    sscanf(buf, "%u//%u", &v, &n);
                    T(numtriangles).vindices[0] = v;
                    T(numtriangles).tindices[0] = -1;
                    T(numtriangles).nindices[0] = n;
                    fscanf(file, "%u//%u", &v, &n);
                    T(numtriangles).vindices[1] = v;
                    T(numtriangles).tindices[1] = -1;
                    T(numtriangles).nindices[1] = n;
                    fscanf(file, "%u//%u", &v, &n);
                    T(numtriangles).vindices[2] = v;
		            T(numtriangles).tindices[2] = -1;
                    T(numtriangles).nindices[2] = n;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%u//%u", &v, &n) > 0) {

                        T(numtriangles).material = material;

                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v;
                        T(numtriangles).tindices[2] = -1;
                        T(numtriangles).nindices[2] = n;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%u/%u/%u", &v, &t, &n) == 3) {
                    /* v/t/n */
					
                    T(numtriangles).vindices[0] = v;
                    T(numtriangles).tindices[0] = t;
                    T(numtriangles).nindices[0] = n;
                    fscanf(file, "%u/%u/%u", &v, &t, &n);
                    T(numtriangles).vindices[1] = v;
                    T(numtriangles).tindices[1] = t;
                    T(numtriangles).nindices[1] = n;
                    fscanf(file, "%u/%u/%u", &v, &t, &n);
                    T(numtriangles).vindices[2] = v;
                    T(numtriangles).tindices[2] = t;
                    T(numtriangles).nindices[2] = n;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%u/%u/%u", &v, &t, &n) > 0) {

                        T(numtriangles).material = material;

                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v;
                        T(numtriangles).tindices[2] = t;
                        T(numtriangles).nindices[2] = n;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%u/%u", &v, &t) == 2) {
                    /* v/t */
                    T(numtriangles).vindices[0] = v;
                    T(numtriangles).tindices[0] = t;
		    T(numtriangles).nindices[0] = -1;
		    fscanf(file, "%u/%u", &v, &t);
                    T(numtriangles).vindices[1] = v;
                    T(numtriangles).tindices[1] = t;
		    T(numtriangles).nindices[1] = -1;
                    fscanf(file, "%u/%u", &v, &t);
                    T(numtriangles).vindices[2] = v;
                    T(numtriangles).tindices[2] = t;
		    T(numtriangles).nindices[2] = -1;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%u/%u", &v, &t) > 0) {

                        T(numtriangles).material = material;

                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v;
                        T(numtriangles).tindices[2] = t;
                        T(numtriangles).nindices[2] = -1;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else {
                    /* v */
                    sscanf(buf, "%u", &v);
                    T(numtriangles).vindices[0] = v;
                    T(numtriangles).tindices[0] = -1;
		    T(numtriangles).nindices[0] = -1;
		    fscanf(file, "%u", &v);
                    T(numtriangles).vindices[1] = v;
                    T(numtriangles).tindices[1] = -1;
		    T(numtriangles).nindices[1] = -1;
                    fscanf(file, "%u", &v);
                    T(numtriangles).vindices[2] = v;
                    T(numtriangles).tindices[2] = -1;
		    T(numtriangles).nindices[2] = -1;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%u", &v) > 0) {

                        T(numtriangles).material = material;

                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v;
                        T(numtriangles).tindices[2] = -1;
                        T(numtriangles).nindices[2] = -1;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                }
                break;
                
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
    }
  }
  //glmWeldVectors(model->vertices,&model->vertices.size(),0.01f);
  
}


/* public functions */


/* glmUnitize: "unitize" a model by translating it to the origin and
 * scaling it to fit in a unit cube around the origin.   Returns the
 * scalefactor used.
 *
 * model - properly initialized ImportedModel structure 
 */
F32
glmUnitize(ImportedModel* model)
{
    F32 maxx, minx, maxy, miny, maxz, minz;
    F32 cx, cy, cz, w, h, d;
    F32 scale;
    
    assert(model);
	assert(!model->vertices.empty());
    
	vector<vertex>::iterator vertIterator;
    /* get the max/mins */
    maxx = minx = model->vertices[0].x;
    maxy = miny = model->vertices[0].y;
    maxz = minz = model->vertices[0].z;
    for (vertIterator = model->vertices.begin()+1; vertIterator != model->vertices.end(); vertIterator++) {
        if (maxx < (*vertIterator).x)
            maxx = (*vertIterator).x;
        if (minx > (*vertIterator).x)
            minx = (*vertIterator).x;
        
        if (maxy < (*vertIterator).y)
            maxy = (*vertIterator).y;
        if (miny > (*vertIterator).y)
            miny = (*vertIterator).y;
        
        if (maxz < (*vertIterator).z)
            maxz = (*vertIterator).z;
        if (minz > (*vertIterator).z)
            minz = (*vertIterator).z;
    }
    
    /* calculate model width, height, and depth */
    w = abs(maxx) + abs(minx);
    h = abs(maxy) + abs(miny);
    d = abs(maxz) + abs(minz);
    
    /* calculate center of the model */
    cx = (maxx + minx) / 2.0f;
    cy = (maxy + miny) / 2.0f;
    cz = (maxz + minz) / 2.0f;
    
    /* calculate unitizing scale factor */
    scale = 2.0f / max(max(w, h), d);
    
    /* translate around center then scale */
    for (vertIterator = model->vertices.begin()+1; vertIterator != model->vertices.end(); vertIterator++) {
        (*vertIterator).x -= cx;
        (*vertIterator).y -= cy;
        (*vertIterator).z -= cz;
        (*vertIterator).x *= scale;
        (*vertIterator).y *= scale;
        (*vertIterator).z *= scale;
    }
    
    return scale;
}

/* glmDimensions: Calculates the dimensions (width, height, depth) of
 * a model.
 *
 * model   - initialized ImportedModel structure
 * dimensions - array of 3 GLfloats (GLfloat dimensions[3])
 */
void
glmDimensions(ImportedModel* model, F32* dimensions)
{
    F32 maxx, minx, maxy, miny, maxz, minz;

    assert(model);
    assert(!model->vertices.empty());
    assert(dimensions);
    
    /* get the max/mins */
    maxx = minx = model->vertices[0].x;
    maxy = miny = model->vertices[0].y;
    maxz = minz = model->vertices[0].z;
	vector<vertex>::iterator vertIterator;
    for (vertIterator = model->vertices.begin()+1; vertIterator != model->vertices.end(); vertIterator++) {
        if (maxx < (*vertIterator).x)
            maxx = (*vertIterator).x;
        if (minx > (*vertIterator).x)
            minx = (*vertIterator).x;
        
        if (maxy < (*vertIterator).y)
            maxy = (*vertIterator).y;
        if (miny > (*vertIterator).y)
            miny = (*vertIterator).y;
        
        if (maxz < (*vertIterator).z)
            maxz = (*vertIterator).z;
        if (minz > (*vertIterator).z)
            minz = (*vertIterator).z;
    }
    
    /* calculate model width, height, and depth */
    dimensions[0] = abs(maxx) + abs(minx);
    dimensions[1] = abs(maxy) + abs(miny);
    dimensions[2] = abs(maxz) + abs(minz);
}

/* glmReverseWinding: Reverse the polygon winding for all polygons in
 * this model.   Default winding is counter-clockwise.  Also changes
 * the direction of the normals.
 * 
 * model - properly initialized ImportedModel structure 
 */
void
glmReverseWinding(ImportedModel* model)
{
    U32 i, swap;
    
    assert(model);
    
    for (i = 0; i < model->triangles.size(); i++) {
        swap = T(i).vindices[0];
        T(i).vindices[0] = T(i).vindices[2];
        T(i).vindices[2] = swap;
        
        if (!model->normals.empty()) {
            swap = T(i).nindices[0];
            T(i).nindices[0] = T(i).nindices[2];
            T(i).nindices[2] = swap;
        }
        
        if (!model->texcoords.empty()) {
            swap = T(i).tindices[0];
            T(i).tindices[0] = T(i).tindices[2];
            T(i).tindices[2] = swap;
        }
    }
    
    /* reverse facet normals */
    for (i = 1; i <= model->facetnorms.size(); i++) {
        model->facetnorms[i].x = -model->facetnorms[i].x;
        model->facetnorms[i].y = -model->facetnorms[i].y;
        model->facetnorms[i].z = -model->facetnorms[i].z;
    }
    
    /* reverse vertex normals */
    for (i = 1; i <= model->normals.size(); i++) {
        model->normals[i].x = -model->normals[i].x;
        model->normals[i].y = -model->normals[i].y;
        model->normals[i].z = -model->normals[i].z;
    }
}

/* glmFacetNormals: Generates facet normals for a model (by taking the
 * cross product of the two vectors derived from the sides of each
 * triangle).  Assumes a counter-clockwise winding.
 *
 * model - initialized ImportedModel structure
 */
void
glmFacetNormals(ImportedModel* model)
{
    GLuint  i;
    GLfloat u[3];
    GLfloat v[3];

    assert(model);
    assert(!model->vertices.empty());
  
    /* clobber any old facetnormals */
    if (!model->facetnorms.empty()) model->facetnorms.clear();
    
    
    for (i = 0; i < model->triangles.size(); i++) 
	{
        model->triangles[i].findex = i+1;		
        
        u[0] = model->vertices[T(i).vindices[1]].x -
            model->vertices[T(i).vindices[0]].x;
        u[1] = model->vertices[T(i).vindices[1]].y -
            model->vertices[T(i).vindices[0]].y;
        u[2] = model->vertices[T(i).vindices[1]].z -
            model->vertices[T(i).vindices[0]].z;
        
        v[0] = model->vertices[T(i).vindices[2]].x -
            model->vertices[T(i).vindices[0]].x;
        v[1] = model->vertices[T(i).vindices[2]].y -
            model->vertices[T(i).vindices[0]].y;
        v[2] = model->vertices[T(i).vindices[2]].z -
            model->vertices[T(i).vindices[0]].z;
        
		vec3 temp(0,0,0);
		temp.cross(vec3(u[0],u[1],u[2]), vec3(v[0],v[1],v[2]));
		model->facetnorms[(i+1)].bind(temp);
		model->facetnorms[(i+1)].normalize();
    }
}

/* glmVertexNormals: Generates smooth vertex normals for a model.
 * First builds a list of all the triangles each vertex is in.   Then
 * loops through each vertex in the the list averaging all the facet
 * normals of the triangles each vertex is in.   Finally, sets the
 * normal index in the triangle for the vertex to the generated smooth
 * normal.   If the dot product of a facet normal and the facet normal
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
GLvoid
glmVertexNormals(ImportedModel* model, GLfloat angle)
{
    Node*    node;
    Node*    tail;
    Node** members;
	U32 numnormals;
    vertex average;
    GLfloat dot, cos_angle;
    GLuint  i, avg;
    
    assert(model);
    assert(!model->facetnorms.empty());
    
    /* calculate the cosine of the angle (in degrees) */
    cos_angle = cos(angle * M_PI / 180.0f);
    
    /* nuke any previous normals */
    if (!model->normals.empty())    model->normals.clear();
    
    /* allocate a structure that will hold a linked list of triangle
    indices for each vertex */
    members = New Node*[model->vertices.size() + 1];
    for (i = 1; i <= model->vertices.size(); i++)
        members[i] = NULL;
    
    /* for every triangle, create a node for each vertex in it */
    for (i = 0; i < model->triangles.size(); i++) {
        node = New Node;
        node->index = i;
        node->next  = members[T(i).vindices[0]];
        members[T(i).vindices[0]] = node;
        
        node = New Node;
        node->index = i;
        node->next  = members[T(i).vindices[1]];
        members[T(i).vindices[1]] = node;
        
        node = New Node;
        node->index = i;
        node->next  = members[T(i).vindices[2]];
        members[T(i).vindices[2]] = node;
    }
    
    /* calculate the average normal for each vertex */
    numnormals = 1;
    for (i = 1; i <= model->vertices.size(); i++) {
    /* calculate an average normal for this vertex by averaging the
        facet normal of every triangle this vertex is in */
        node = members[i];
        if (!node)
            cout << "glmVertexNormals(): vertex w/o a triangle" << endl;
        average.x = 0.0; average.y = 0.0; average.z = 0.0;
        avg = 0;
        while (node) {
        /* only average if the dot product of the angle between the two
        facet normals is greater than the cosine of the threshold
        angle -- or, said another way, the angle between the two
            facet normals is less than (or equal to) the threshold angle */
            dot = model->facetnorms[T(node->index).findex].dot(
                model->facetnorms[T(members[i]->index).findex]);
            if (dot > cos_angle) {
                node->averaged = GL_TRUE;
                average.x += model->facetnorms[T(node->index).findex].x;
                average.z += model->facetnorms[T(node->index).findex].z;
                avg = 1;            /* we averaged at least one normal! */
            } else {
                node->averaged = GL_FALSE;
            }
            node = node->next;
        }
        
        if (avg) {
            /* normalize the averaged normal */
			average.normalize();
            
            /* add the normal to the vertex normals list */
            model->normals.push_back(average);
            avg = numnormals;
            numnormals++;
        }
        
        /* set the normal of this vertex in each triangle it is in */
        node = members[i];
        while (node) 
		{
            if (node->averaged) {
				
                /* if this node was averaged, use the average normal */
                if (T(node->index).vindices[0] == i)
                    T(node->index).nindices[0] = avg;
                else if (T(node->index).vindices[1] == i)
                    T(node->index).nindices[1] = avg;
                else if (T(node->index).vindices[2] == i)
                    T(node->index).nindices[2] = avg;
            } else {
				
                /* if this node wasn't averaged, use the facet normal */
                model->normals[numnormals].x = 
                    model->facetnorms[T(node->index).findex].x;
                model->normals[numnormals].y = 
                    model->facetnorms[T(node->index).findex].y;
                model->normals[numnormals].z = 
                    model->facetnorms[T(node->index).findex].z;
                if (T(node->index).vindices[0] == i)
                    T(node->index).nindices[0] = numnormals;
                else if (T(node->index).vindices[1] == i)
                    T(node->index).nindices[1] = numnormals;
                else if (T(node->index).vindices[2] == i)
                    T(node->index).nindices[2] = numnormals;
                numnormals++;
            }
            node = node->next;
        }
    }
    

    /* free the member information */
    for (i = 1; i <= model->vertices.size(); i++) {
        node = members[i];
        while (node) {
            tail = node;
            node = node->next;
            delete tail;
        }
    }
    delete members;
    delete node;
}

GLvoid
glmLinearTexture(ImportedModel* model)
{
    Group *group;
    GLfloat dimensions[3];
    GLfloat x, y, scalefactor;
    GLuint i;
    
    assert(model);
    
    if (!model->texcoords.empty()) model->texcoords.clear();
    
    glmDimensions(model, dimensions);
    scalefactor = 2.0f / 
        abs(max(max(dimensions[0], dimensions[1]), dimensions[2]));
    
    /* do the calculations */
	vector<vertex>::iterator vertIterator;
	texCoord tempT;
	i = 0;
    for(vertIterator = model->vertices.begin(); vertIterator != model->vertices.end(); vertIterator++,i++) {
        x = (*vertIterator).x * scalefactor;
        y = (*vertIterator).y * scalefactor;
        tempT.u = (x + 1.0f) / 2.0f;
        tempT.v = (y + 1.0f) / 2.0f;
		model->texcoords.push_back(tempT);
    }
    
    /* go through and put texture coordinate indices in all the triangles */
    group = &model->groups[0];
    while(group) {
        for(i = 0; i < group->numtriangles; i++) {
            T(group->triangles[i]).tindices[0] = T(group->triangles[i]).vindices[0];
            T(group->triangles[i]).tindices[1] = T(group->triangles[i]).vindices[1];
            T(group->triangles[i]).tindices[2] = T(group->triangles[i]).vindices[2];
        }    
        group = group->next;
    }
    
}

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
GLvoid
glmSpheremapTexture(ImportedModel* model)
{
    GLfloat theta, phi, rho, x, y, z, r;
    GLuint i;
	texCoord tempT;
    
    assert(model);
    assert(!model->normals.empty());
    
    if (!model->texcoords.empty()) model->texcoords.clear();
    
    for (i = 1; i <= model->normals.size(); i++) {
        z = model->normals[i].x;  /* re-arrange for pole distortion */
        y = model->normals[i].y;
        x = model->normals[i].z;
        r = sqrt((x * x) + (y * y));
        rho = sqrt((r * r) + (z * z));
        
        if(r == 0.0) {
            theta = 0.0;
            phi = 0.0;
        } else {
            if(z == 0.0)
                phi = 3.14159265f / 2.0f;
            else
                phi = acos(z / rho);
            
            if(y == 0.0)
                theta = 3.141592365f / 2.0f;
            else
                theta = asin(y / r) + (3.14159265f / 2.0f);
        }
        tempT.u = theta / 3.14159265f;
		tempT.v = phi / 3.14159265f;
        model->texcoords.push_back(tempT);
    }
    
    /* go through and put texcoord indices in all the triangles */
     for(vector<Group>::iterator groupIterator = model->groups.begin(); groupIterator != model->groups.end(); groupIterator++) {
        for (i = 0; i < (*groupIterator).numtriangles; i++) {
            T((*groupIterator).triangles[i]).tindices[0] = T((*groupIterator).triangles[i]).nindices[0];
            T((*groupIterator).triangles[i]).tindices[1] = T((*groupIterator).triangles[i]).nindices[1];
            T((*groupIterator).triangles[i]).tindices[2] = T((*groupIterator).triangles[i]).nindices[2];
        }
    }
}

/* glmDelete: Deletes a ImportedModel structure.
 *
 * model - initialized ImportedModel structure
 */
GLvoid
glmDelete(ImportedModel* model, bool keepTextures)
{
    GLuint i;
    
    assert(model);
    
    if (model->pathname)   free(model->pathname);
    if (model->mtllibname) free(model->mtllibname);
    if (!model->vertices.empty())   model->vertices.clear();
    if (!model->normals.empty())    model->normals.clear();
    if (!model->texcoords.empty())  model->texcoords.clear();
    if (!model->facetnorms.empty()) model->facetnorms.clear();
    if (!model->triangles.empty())  model->triangles.clear();

	if(!keepTextures)
	{
		if (!model->materials.empty()) {
			for (i = 0; i < model->materials.size(); i++)
				model->materials[i].name.empty();
			model->materials.clear();
		}
		if (model->textures.size() > 0) {
			for (model->TextureIterator = model->textures.begin(); model->TextureIterator != model->textures.end(); model->TextureIterator++) {
				(*model->TextureIterator).name.empty();
				glDeleteTextures(1,&(*model->TextureIterator).id);
			}
			model->textures.clear();
		}
	}

    for(vector<Group>::iterator groupIterator = model->groups.begin(); groupIterator != model->groups.end(); groupIterator++) {
        delete (*groupIterator).name;
        delete (*groupIterator).triangles;
    }
	model->groups.clear();
    
    if(!keepTextures) delete model;
}


GLvoid glmDraw(ImportedModel* model, GLuint mode, GLuint ListID)
{
	glmDraw(model,mode,0,ListID);
}

GLvoid glmDraw(ImportedModel* model, GLuint mode,char *drawonly, GLuint ListID)
{
    GLuint i, j,blenditer, newmaterial, newtexture;
	GLuint blendmodel = 0;
    triangle* triangle;
    
    
    GLuint material, map_diffuse;
	Material* materialp;
    assert(model);
    assert(!model->vertices.empty());
    model->ListID = ListID;
    
    if (mode & GLM_FLAT && !model->facetnorms.empty()) {
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_SMOOTH && !model->normals.empty()) {
        mode &= ~GLM_SMOOTH;
    }
    if (mode & GLM_TEXTURE && !model->texcoords.empty()) {
        mode &= ~GLM_TEXTURE;
    }
    if (mode & GLM_FLAT && mode & GLM_SMOOTH) {
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_COLOR && !model->materials.empty()) {
        mode &= ~GLM_COLOR;
    }
    if (mode & GLM_MATERIAL && !model->materials.empty()) {
        mode &= ~GLM_MATERIAL;
    }
    if (mode & GLM_COLOR && mode & GLM_MATERIAL) {
        mode &= ~GLM_COLOR;
    }
    if (mode & GLM_COLOR)
        glEnable(GL_COLOR_MATERIAL);
    else if (mode & GLM_MATERIAL)
        glDisable(GL_COLOR_MATERIAL);

    if (mode & GLM_TEXTURE) {
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

    if(mode & GLM_2_SIDED)
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE); 
    else
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE); 

    for(blenditer = 0; blenditer<2; blenditer++) {
	int blending = 0;
	newmaterial = 0;
	material = -1;
	materialp = NULL;
	newtexture = 0;
	map_diffuse = -1;	/* default material */
	for(vector<Group>::iterator groupIterator = model->groups.begin(); groupIterator != model->groups.end(); groupIterator++)
	{
	    if (mode & (GLM_MATERIAL|GLM_COLOR|GLM_TEXTURE)) {
		material = (*groupIterator).material;
		materialp = &model->materials[material];
		blending = (materialp->diffuse[3] < 1.0f);
		blendmodel |= blending;
		newmaterial = 1;
			if(materialp->textureId != map_diffuse) {
			    newtexture = 1;
			    map_diffuse = materialp->textureId;
			}
	    }
        
	    glBegin(GL_TRIANGLES);
	    for (i = 0; i < (*groupIterator).numtriangles; i++) {
		triangle = &T((*groupIterator).triangles[i]);

		if (mode & (GLM_MATERIAL|GLM_COLOR|GLM_TEXTURE)) {
		    // if the triangle has a different material than the last drawn triangle 
		    if(triangle->material && triangle->material != material) {
			material = triangle->material;
			materialp = &model->materials[material];
			blending = (materialp->diffuse[3] < 1.0);
			blendmodel |= blending;
			newmaterial = 1;
			if(materialp->textureId != map_diffuse) {
			    newtexture = 1;
			    map_diffuse = materialp->textureId;
			}
		    }
		}

		/* render only if in the right blending pass */
		if(blending == blenditer) {
		    if(newmaterial) {
			newmaterial = 0;
			if (mode & GLM_TEXTURE) {
			    if(newtexture) {
				newtexture = 0;
				glEnd();
				if(map_diffuse == -1)
				    glBindTexture(GL_TEXTURE_2D, 0);
				else
				{
					for (model->TextureIterator = model->textures.begin(); model->TextureIterator != model->textures.end(); model->TextureIterator++) {
					   if ((*model->TextureIterator).temp == map_diffuse)
					   {
							glBindTexture(GL_TEXTURE_2D, (*model->TextureIterator).id);
							continue;
					   }
					}
					
				}
				glBegin(GL_TRIANGLES);
			    }
			}
			if (mode & GLM_MATERIAL) {
			    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, materialp->ambient);
			    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialp->diffuse);
			    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialp->specular);
			    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, materialp->shininess);
			}        
			if (mode & GLM_COLOR) {
				GFXDevice::getInstance().setColor(materialp->diffuse);
			}
		    }
		
		    if (mode & GLM_FLAT)
				glNormal3f(model->facetnorms[triangle->findex].x,
						   model->facetnorms[triangle->findex].y,
						   model->facetnorms[triangle->findex].z);
		
			for (j=0; j<3; j++) {
					if (mode & GLM_SMOOTH && (triangle->nindices[j]!=-1)) {
						assert(triangle->nindices[j]>=1 && triangle->nindices[j]<=model->normals.size());
						glNormal3f(model->normals[triangle->nindices[j]].x,
								model->normals[triangle->nindices[j]].y,
								model->normals[triangle->nindices[j]].z);
					}
					if (mode & GLM_TEXTURE && (triangle->tindices[j]!=-1) && map_diffuse != -1) {
						assert(map_diffuse >= 0 && map_diffuse < model->textures.size());
						assert(triangle->tindices[j]>=1 && triangle->tindices[j]<=model->texcoords.size());
						glTexCoord2f(model->texcoords[triangle->tindices[j]].u,
									 model->texcoords[triangle->tindices[j]].v);

					}
						assert(triangle->vindices[j]>=1 && triangle->vindices[j]<=model->vertices.size());
						glVertex3f(model->vertices[triangle->vindices[j]].x,
								   model->vertices[triangle->vindices[j]].y,
								   model->vertices[triangle->vindices[j]].z);
					}
				}
            
			}
			glEnd();
        
		}
		if(!blendmodel)
		    break;			/* jump out of the for(blenditer) */
		assert(blendmodel);
		/* Prep for second pass with alpha items */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE); /* Type Of Blending To Perform */
		glDepthMask(GL_FALSE);	/* Turn off depth mask */
    }  
    if(blendmodel) {
		glDepthMask(GL_TRUE);	/* DISABLE Blending conditions */
		glDisable(GL_BLEND);
    }
}

/* glmList: Generates and returns a display list for the model using
 * the mode specified.
 *
 * model - initialized ImportedModel structure
 * mode  - a bitwise OR of values describing what is to be rendered.
 *             GLM_NONE     -  render with only vertices
 *             GLM_FLAT     -  render with facet normals
 *             GLM_SMOOTH   -  render with vertex normals
 *             GLM_TEXTURE  -  render with texture coords
 *             GLM_COLOR    -  render with colors (color material)
 *             GLM_MATERIAL -  render with materials
 *             GLM_COLOR and GLM_MATERIAL should not both be specified.  
 * GLM_FLAT and GLM_SMOOTH should not both be specified.  
 */
GLuint
glmList(ImportedModel* model, GLuint mode)
{
    GLuint list;
    
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
	GFXDevice::getInstance().pushMatrix();
	if(model->vegetation)
	{
		GFXDevice::getInstance().translate(model->position.x,model->position.y,model->position.z);
		GFXDevice::getInstance().rotate(model->rotation.x,1.0f,0.0f,0.0f);
		GFXDevice::getInstance().rotate(model->rotation.y,0.0f,1.0f,0.0f);
		GFXDevice::getInstance().rotate(model->rotation.z,0.0f,0.0f,1.0f);
		GFXDevice::getInstance().scale(model->ScaleFactor,model->ScaleFactor,model->ScaleFactor);
    }
    glmDraw(model, mode, list);
	GFXDevice::getInstance().popMatrix();
    glEndList();
    
    return list;
}

void CreateSimpleFaceArray(ImportedModel *pMesh)
{
	for(U32 i = 0; i < pMesh->triangles.size(); i++)
	{
		pMesh->SimpleFaceArray[i].Vertex[0] = pMesh->triangles[i].vindices[0];
		pMesh->SimpleFaceArray[i].Vertex[1] = pMesh->triangles[i].vindices[1];
		pMesh->SimpleFaceArray[i].Vertex[2] = pMesh->triangles[i].vindices[2];
	}
}