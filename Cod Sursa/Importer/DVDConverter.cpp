#include "DVDConverter.h"

#include <assimp.hpp>      // C++ importer interface
#include <aiPostProcess.h> // Post processing flags
#include "Managers/ResourceManager.h"
#include "TextureManager/Texture2D.h"

DVDFile::DVDFile()
{
	_ppsteps = aiProcess_CalcTangentSpace         | // calculate tangents and bitangents if possible
			   aiProcess_JoinIdenticalVertices    | // join identical vertices/ optimize indexing
			   aiProcess_ValidateDataStructure    | // perform a full validation of the loader's output
			   aiProcess_ImproveCacheLocality     | // improve the cache locality of the output vertices
			   aiProcess_RemoveRedundantMaterials | // remove redundant materials
			   aiProcess_FindDegenerates          | // remove degenerated polygons from the import
			   aiProcess_FindInvalidData          | // detect invalid model data, such as invalid normal vectors
			   aiProcess_GenUVCoords              | // convert spherical, cylindrical, box and planar mapping to proper UVs
			   aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
			   aiProcess_FindInstances            | // search for instanced meshes and remove them by references to one master
			   aiProcess_LimitBoneWeights         | // limit bone weights to 4 per vertex
			   aiProcess_OptimizeMeshes	          | // join small meshes, if possible;
			   0;
	_shouldDelete = false;
}

DVDFile::~DVDFile()
{
	unload();
}

bool DVDFile::unload()
{
	for(_subMeshIterator = getSubMeshes().begin(); _subMeshIterator != getSubMeshes().end(); _subMeshIterator++)
	{
		SubMesh* s = (*_subMeshIterator);
		if(s->unload())	
		{
			delete s;
			s= NULL;
		}
		else return false;
	}
	getSubMeshes().clear();
	if(getShader()) delete getShader();
	else return false;

	return true;
}
bool DVDFile::load(const string& file)
{
	bool removeLinesAndPoints = true;
	getName() = file;
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE , removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT : 0 );
	
	scene = importer.ReadFile( file, _ppsteps                    |
								aiProcess_Triangulate            |
								aiProcess_SplitLargeMeshes		 |
								aiProcess_SortByPType            |
								aiProcess_GenSmoothNormals);

	if( !scene)
	{
		cout << "DVDFile::load(" << file << "): " << importer.GetErrorString();
		return false;
	}
	U32 index=0;
	for(U32 n = 0; n < scene->mNumMeshes; n++)
	{
		string temp;
		char a;
		for(U32 j = 0; j < scene->mMeshes[n]->mName.length; j++)
		{
			a = scene->mMeshes[n]->mName.data[j];
			temp += a;
		}

		index = getSubMeshes().size();
		addSubMesh(new SubMesh(temp));

		for(U32 j = 0; j < scene->mMeshes[n]->mNumVertices; j++)
		{
			getSubMeshes()[index]->getGeometryVBO()->getPosition().push_back(vec3(scene->mMeshes[n]->mVertices[j].x,
																				  scene->mMeshes[n]->mVertices[j].y,
																				  scene->mMeshes[n]->mVertices[j].z));
			getSubMeshes()[index]->getGeometryVBO()->getNormal().push_back(vec3(scene->mMeshes[n]->mNormals[j].x,
																				scene->mMeshes[n]->mNormals[j].y,
																				scene->mMeshes[n]->mNormals[j].z));
		}

		if(scene->mMeshes[n]->mTextureCoords[0] != NULL)
			for(U32 j = 0; j < scene->mMeshes[n]->mNumVertices; j++)
			{
				getSubMeshes()[index]->getGeometryVBO()->getTexcoord().push_back(vec2(scene->mMeshes[n]->mTextureCoords[0][j].x,
																						scene->mMeshes[n]->mTextureCoords[0][j].y));
				getSubMeshes()[index]->getGeometryVBO()->getTangent().push_back(vec3(scene->mMeshes[n]->mTangents[j].x,
																					   scene->mMeshes[n]->mTangents[j].y,
																					   scene->mMeshes[n]->mTangents[j].z));
			}

		for(U32 k = 0; k < scene->mMeshes[n]->mNumFaces; k++)
		  for(U32 m = 0; m < scene->mMeshes[n]->mFaces[k].mNumIndices; m++)
			getSubMeshes()[index]->getIndices().push_back(scene->mMeshes[n]->mFaces[k].mIndices[m]);
		  
		getSubMeshes()[index]->getGeometryVBO()->Create(GL_STATIC_DRAW);
	 
		aiMaterial *mat = scene->mMaterials[scene->mMeshes[n]->mMaterialIndex];
		aiReturn result;  aiString tName; aiTextureMapping mapping; unsigned int uvInd;
		float blend;  aiTextureOp op; aiTextureMapMode mode[3];

		result = mat->GetTexture(aiTextureType_DIFFUSE, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
		if(result != AI_SUCCESS)	return NULL;

		string path = tName.data;
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		string pathName = file.substr( 0, file.rfind("/")+1 );
		getSubMeshes()[index]->getMaterial().texture = ResourceManager::getInstance().LoadResource<Texture2DFlipped>(pathName + "../texturi/"  + img_name);
	}
	_render = true;
	return _render;
}
