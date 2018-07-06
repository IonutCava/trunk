/*    
      glm.c
      Nate Robins, 1997, 2000
      nate@pobox.com, http://www.pobox.com/~nate
 
      Wavefront OBJ model file format reader/writer/manipulator.
*/

#include "OBJ.h"
#include "Managers/TextureManager.h"
#include "Managers/ResourceManager.h"
#include "Hardware/Video/GFXDevice.h"
#include "Managers/TerrainManager.h"
#include "IL/il.h"

#define GLM_MAX_SHININESS 100.0
#define total_textures 5
#define MATERIAL_BY_FACE
#define T(x) (model->triangles[(x)])


namespace OBJ
{

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
    
    copies = new vertex[*numvectors];
    memcpy(copies, vectors, (sizeof(vertex) * (*numvectors)));
    
    copied = 1;
    for (i = 0; i < *numvectors; i++) {
        for (j = 0; j <= copied; j++) {
            if (vectors[3 * i].compare(copies[3 * j], epsilon)) {
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

/* glmFindGroup: Find a group in the model */
static Group*
glmFindGroup(ImportedModel* model, const string& name)
{
    vector<Group*>::iterator groupIterator;
	for(groupIterator = model->groups.begin(); groupIterator != model->groups.end(); groupIterator++)
        if (name.compare((*groupIterator)->name) == 0)  return (*groupIterator);
    return NULL;
}

/* glmAddGroup: Add a group to the model */
static Group*
glmAddGroup(ImportedModel* model, const string& name)
{
    Group* group;
    
    group = glmFindGroup(model, name);
	tr1::unordered_map<string,SubMesh*>::iterator it = model->getSubMeshes().find(name);
	bool foundSubMesh = it != (model->getSubMeshes().end());

    if (!group) {
        group = New Group;
		group->name = _strdup(name.c_str());
        group->material = 0;
        group->numtriangles = 0;
        group->triangles = NULL;
        model->groups.push_back(group);
    }
	if(!foundSubMesh)
	{
		if(model->getSubMeshes().size() == 1)
		{
			//model->getSubMeshes().begin()->first = name;
			model->getSubMeshes().begin()->second->getName() = name;
		}
		else
			model->addSubMesh(New SubMesh(name));    
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
			   return i;
    }
    return 0;
    
}

/* glmFindTexture: Find a texture in the model */
static GLuint
glmFindOrAddTexture(ImportedModel* model, std::string name)
{
	ResourceManager& res = ResourceManager::getInstance();
	U32 size = model->textures.size();
	model->textures[size] = (Texture2D*)res.LoadResource<Texture2DFliped>(model->pathname + name);
    cout << "Allocated texture " << size << "(id=" << model->textures[size]->getHandle() << ")" << endl;
    return size;

}

/* glmReadMTL: read a wavefront material library file
 *
 * model - properly initialized ImportedModel structure
 * name  - name of the material library
 */
char* glmStrStrip(const char *s)
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
    rets = (char*)malloc(len+1); /* add a trailing 0 */
    memcpy(rets, s + first, len);
    rets[len] = 0;
    return rets;
}

static GLvoid
glmReadMTL(ImportedModel* model, const std::string& name)
{
    FILE* file;
    char* filename;
    char    buf[128];
    GLuint nummaterials, i;
    
   
    file = fopen(name.c_str(), "r");
    if (!file) {
        cout << "glmReadMTL() failed: can't open material file [ "<< name << " ] " << endl;
			 
    }
    
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
		cout << "glmReadMTL: Got " <<  buf << " instead of \"newmtl\" in file " << name << endl;
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
            filename = new char[FILENAME_MAX];
			fgets(filename, FILENAME_MAX, file);
			filename  = glmStrStrip(filename);
            if(strncmp(buf, "map_Kd", 6) == 0) {
				model->materials[nummaterials].textureId = glmFindOrAddTexture(model, string(filename));
			}else{
				fgets(buf, sizeof(buf), file);
            }
			delete[] filename;
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
    
    U32    v, n, t;
    char   buf[128];
	vertex tempV;
	normal tempN;
	texCoord tempT; 

	/*U32	indFacePosition;
	U32	indFaceNormal;
	U32	indFaceTexcoord;*/

	std::vector<vec3>	tTempPosition;
	std::vector<vec3>	tTempNormal;
	std::vector<vec2>	tTempTexcoord;
	if(model->getSubMeshes().size() == 0) model->addSubMesh(new SubMesh("default"));

	std::vector<vec3>&	tPosition	= model->getSubMeshes().begin()->second->getGeometryVBO()->getPosition();
	std::vector<vec3>&	tNormal		= model->getSubMeshes().begin()->second->getGeometryVBO()->getNormal();
	std::vector<vec2>&	tTexcoord	= model->getSubMeshes().begin()->second->getGeometryVBO()->getTexcoord();

    Group* group = NULL;
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
				tTempPosition.push_back(tempV);
                break;
            case 'n':           /* normal */
                fscanf(file, "%f %f %f", 
                    &tempN.x,
                    &tempN.y, 
                    &tempN.z);
                model->normals.push_back(tempN);
				tTempNormal.push_back(tempN);
                break;
            case 't':           /* texcoord */
                fscanf(file, "%f %f", 
                    &tempT.u,
                    &tempT.v);
				model->texcoords.push_back(tempT);
				tTempTexcoord.push_back(vec2(tempT.u,tempT.v));
                break;
            }
            break;
       	case 'm':
			fgets(buf, sizeof(buf), file);
			sscanf(buf, "%s %s", buf, buf);
			model->mtllibname = ((char*)buf);
			glmReadMTL(model, model->pathname + model->mtllibname);
			break;
		case 'g':               /* group */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			buf[strlen(buf)-1] = '\0';  /* nuke '\n' */
			group = glmAddGroup(model, string(buf));
			break;
		case 'f':               /* face */
			if(!group)	group = glmAddGroup(model, string("default"));

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
   /* allocate memory for the triangles in each group */
	for(vector<Group*>::iterator groupIterator = model->groups.begin(); groupIterator != model->groups.end(); groupIterator++)
	{
      (*groupIterator)->triangles = New GLuint[(*groupIterator)->numtriangles];
	  cout << "Group \"" << (*groupIterator)->name << "\" has " << (*groupIterator)->numtriangles << " triangles" << endl;
	  (*groupIterator)->numtriangles = 0;
	}
	rewind(file);
}

void glmComputeBoundingBox(ImportedModel* model)
{
	model->getBoundingBox().min = vec3(100000.0f, 100000.0f, 100000.0f);
	model->getBoundingBox().max = vec3(-100000.0f, -100000.0f, -100000.0f);
	
	for(int i=0; i<(int)model->vertices.size(); i++)
	{
		model->getBoundingBox().Add( model->vertices[i]);
	}

}

void glmSecondPass(ImportedModel* model, FILE* file) 
{
    GLuint  numtriangles = 0;       /* number of triangles in model */
    Group* group = model->groups[0];               /* current group pointer */
	SubMesh* subMesh = model->getSubMesh(0);
    GLuint  material = 0;           /* current material */
    GLuint  v, n, t;
    char        buf[128];
    triangle tempTr;
       /* on the second pass through the file, read all the data into the
    allocated arrays */
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
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
                buf[strlen(buf)-1] = '\0';  /* nuke '\n' */
                group = glmFindGroup(model, string(buf));
				subMesh = model->getSubMesh(string(buf));
                break;
            case 'f':               /* face */
                v = n = t = 0;
				model->triangles.push_back(tempTr);
	         	T(numtriangles).findex = -1;
                if(group->material == 0)
                    group->material = material;
                T(numtriangles).material = material;
                fscanf(file, "%s", buf);
                /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
                if (strstr(buf, "//")) {
                    /* v//n */
                    sscanf(buf, "%u//%u", &v, &n);
                    T(numtriangles).vindices[0] = v-1;
                    T(numtriangles).tindices[0] = -1;
                    T(numtriangles).nindices[0] = n-1;
                    fscanf(file, "%u//%u", &v, &n);
                    T(numtriangles).vindices[1] = v-1;
                    T(numtriangles).tindices[1] = -1;
                    T(numtriangles).nindices[1] = n-1;
                    fscanf(file, "%u//%u", &v, &n);
                    T(numtriangles).vindices[2] = v-1;
		            T(numtriangles).tindices[2] = -1;
                    T(numtriangles).nindices[2] = n-1;
                    group->triangles[group->numtriangles++] = numtriangles;
					//subMesh->getGeometryVBO()->getPosition().push_back(vec3());
                    numtriangles++;
                    while(fscanf(file, "%u//%u", &v, &n) > 0) {

                        T(numtriangles).material = material;

                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v-1;
                        T(numtriangles).tindices[2] = -1;
                        T(numtriangles).nindices[2] = n-1;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%u/%u/%u", &v, &t, &n) == 3) {
                    /* v/t/n */
					
                    T(numtriangles).vindices[0] = v-1;
                    T(numtriangles).tindices[0] = t-1;
                    T(numtriangles).nindices[0] = n-1;
                    fscanf(file, "%u/%u/%u", &v, &t, &n);
                    T(numtriangles).vindices[1] = v-1;
                    T(numtriangles).tindices[1] = t-1;
                    T(numtriangles).nindices[1] = n-1;
                    fscanf(file, "%u/%u/%u", &v, &t, &n);
                    T(numtriangles).vindices[2] = v-1;
                    T(numtriangles).tindices[2] = t-1;
                    T(numtriangles).nindices[2] = n-1;
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
                        T(numtriangles).vindices[2] = v-1;
                        T(numtriangles).tindices[2] = t-1;
                        T(numtriangles).nindices[2] = n-1;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%u/%u", &v, &t) == 2) {
                    /* v/t */
                    T(numtriangles).vindices[0] = v-1;
                    T(numtriangles).tindices[0] = t-1;
					T(numtriangles).nindices[0] = -1;

					fscanf(file, "%u/%u", &v, &t);
                    T(numtriangles).vindices[1] = v-1;
                    T(numtriangles).tindices[1] = t-1;
					T(numtriangles).nindices[1] = -1;

                    fscanf(file, "%u/%u", &v, &t);
                    T(numtriangles).vindices[2] = v-1;
                    T(numtriangles).tindices[2] = t-1;
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
                        T(numtriangles).vindices[2] = v-1;
                        T(numtriangles).tindices[2] = t-1;
                        T(numtriangles).nindices[2] = -1;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else {
                    /* v */
                    sscanf(buf, "%u", &v);
                    T(numtriangles).vindices[0] = v-1;
                    T(numtriangles).tindices[0] = -1;
					T(numtriangles).nindices[0] = -1;

					fscanf(file, "%u", &v);
                    T(numtriangles).vindices[1] = v-1;
                    T(numtriangles).tindices[1] = -1;
					T(numtriangles).nindices[1] = -1;

                    fscanf(file, "%u", &v);
                    T(numtriangles).vindices[2] = v-1;
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
                        T(numtriangles).vindices[2] = v-1;
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
  fclose(file);
  CreateSimpleFaceArray(model);
  glmComputeBoundingBox(model);
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
    for (i = 0; i < model->facetnorms.size(); i++) {
        model->facetnorms[i].x = -model->facetnorms[i].x;
        model->facetnorms[i].y = -model->facetnorms[i].y;
        model->facetnorms[i].z = -model->facetnorms[i].z;
    }
    
    /* reverse vertex normals */
    for (i = 0; i < model->normals.size(); i++) {
        model->normals[i].x = -model->normals[i].x;
        model->normals[i].y = -model->normals[i].y;
        model->normals[i].z = -model->normals[i].z;
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
    
    if (!model->pathname.empty())   model->pathname.clear();
    if (!model->mtllibname.empty()) model->mtllibname.clear();
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
			for (model->textureIterator = model->textures.begin(); model->textureIterator != model->textures.end(); model->textureIterator++) {
				(*model->textureIterator).second->Destroy();
			}
			model->textures.clear();
		}
	}

    for(vector<Group*>::iterator groupIterator = model->groups.begin(); groupIterator != model->groups.end(); groupIterator++) {
        delete (*groupIterator)->name;
        delete (*groupIterator)->triangles;
    }
	model->groups.clear();
    
    if(!keepTextures) delete model;
}
/*
	ToDo: Remove display list -> move to VBO
*/
U32 glmList(ImportedModel* model, GLuint mode)
{
    GLuint list;
    
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glmDraw(model, mode);
    glEndList();
    
    return list;
}
/*
	ToDo: Remove display list -> move to VBO
*/
void glmDraw(ImportedModel* model, GLuint mode)
{
    GLuint i, j, newmaterial, newtexture;
	GLuint blendmodel = 0;
    triangle* triangle;
    
    GLuint material, map_diffuse;
	Material* materialp;
    assert(model);
    assert(!model->vertices.empty());
    
	newmaterial = 0;
	material = -1;
	materialp = NULL;
	newtexture = 0;
	map_diffuse = -1;	/* default material */

	for(vector<Group*>::iterator groupIterator = model->groups.begin(); groupIterator != model->groups.end(); groupIterator++)
	{
		if (mode & (GLM_MATERIAL|GLM_COLOR|GLM_TEXTURE)) 
		{
			material = (*groupIterator)->material;
			materialp = &model->materials[material];
			newmaterial = 1;
			if(materialp->textureId != map_diffuse) {
				newtexture = 1;
				map_diffuse = materialp->textureId;
			}
		}
      
		glBegin(GL_TRIANGLES);
		for (i = 0; i < (*groupIterator)->numtriangles; i++) {
			triangle = &T((*groupIterator)->triangles[i]);
			if (mode & (GLM_MATERIAL|GLM_COLOR|GLM_TEXTURE)) {
				// if the triangle has a different material than the last drawn triangle 
				if(triangle->material && triangle->material != material) 
				{
					material = triangle->material;
					materialp = &model->materials[material];
					newmaterial = 1;
					if(materialp->textureId != map_diffuse) {
						newtexture = 1;
						map_diffuse = materialp->textureId;
					}
				}
			}

			if(newmaterial){
				newmaterial = 0;
				if(newtexture){
					newtexture = 0;
					glEnd();
					if(map_diffuse == -1)
					{
						glBindTexture(GL_TEXTURE_2D, 0);
						model->getShader()->Uniform("texture",false);
					}
					else
					{
						model->textures[map_diffuse]->Bind(0);
						model->getShader()->Uniform("texture",true);
						//glBindTexture(GL_TEXTURE_2D, model->textures[map_diffuse].id);
						model->getShader()->UniformTexture("texDiffuse",0);
					}
					
					glBegin(GL_TRIANGLES);
				}
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, materialp->ambient);
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialp->diffuse);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialp->specular);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, materialp->shininess);
			}
		
		
			for (j=0; j<3; j++) {
				if ((triangle->nindices[j]!=-1)) {
					 glNormal3f(model->normals[triangle->nindices[j]].x,
								model->normals[triangle->nindices[j]].y, 
								model->normals[triangle->nindices[j]].z);
				}
				if ((triangle->tindices[j]!=-1) && map_diffuse != -1) {
					glTexCoord2f(model->texcoords[triangle->tindices[j]].u,
								 model->texcoords[triangle->tindices[j]].v);
				}
				glVertex3f(model->vertices[triangle->vindices[j]].x,
						   model->vertices[triangle->vindices[j]].y,
						   model->vertices[triangle->vindices[j]].z);

			} //for (j)
		}//groupIter->numtriangles
	
		glEnd();
		//if(map_diffuse != -1) /*model->textures[map_diffuse]->Unbind(0);//*/glBindTexture(GL_TEXTURE_2D,0);

	}//groupIter
}

void CreateSimpleFaceArray(ImportedModel *pMesh)
{
	simpletriangle tempSF;
	for(U32 i = 0; i < pMesh->triangles.size(); i++)
	{
		pMesh->SimpleFaceArray.push_back(tempSF);
		pMesh->SimpleFaceArray[i].Vertex[0] = pMesh->triangles[i].vindices[0];
		pMesh->SimpleFaceArray[i].Vertex[1] = pMesh->triangles[i].vindices[1];
		pMesh->SimpleFaceArray[i].Vertex[2] = pMesh->triangles[i].vindices[2];
	}
}

}
