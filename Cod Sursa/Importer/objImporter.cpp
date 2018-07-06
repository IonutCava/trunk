#include "objImporter.h"
#include "OBJ.h"
#include "Managers/ResourceManager.h"

ImportedModel* ReadOBJ(string filename,vec3 position, F32 scale,vec3 rotation, bool vegetation)
{
	return ReadOBJ(filename,position,scale,rotation,vegetation,0);
}

ImportedModel* ReadOBJ(string filename,vec3 position, F32 scale,vec3 rotation, bool vegetation,mycallback *call)
{

    ImportedModel* model;
    FILE*   file;
	filename = "assets/modele/"+filename;

	/*
	**	Open the file for reading
	*/
	fopen_s(&file,filename.c_str(),"r");
    if (!file) {
		return NULL;
    }
    cout << "Se incarca modelul '" << filename << "'" << endl;
    /* allocate a new model */

    model = New ImportedModel;
    model->pathname    = (char*)filename.c_str();
    model->mtllibname    = NULL;
    model->position      = position;
	model->ListID        = -1;
    model->ModelName = filename;
	model->NormalMapName = "not_used";
	model->IdNormalMap = -1;
	model->ScaleFactor = scale;
	model->rotation  = rotation;
	model->vegetation = vegetation;
	/*model->shader = (Shader*)ResourceManager::getInstance().LoadResource(ResourceManager::SHADER, "lighting");*/

	GLM_BLENDING = 0;


    glmFirstPass(model, file);
    
    /* allocate memory */
    model->vertices.clear();
    rewind(file);
    glmSecondPass(model, file);
    CreateSimpleFaceArray(model);
    /* close the file */
    fclose(file);
    
	//geometry.getGeometryVBO()
    return model;
}






