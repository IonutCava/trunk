#include "DVDConverter.h"

#include <assimp.hpp>      // C++ importer interface
#include <aiPostProcess.h> // Post processing flags
#include "Managers/ResourceManager.h"
#include "TextureManager/Texture2D.h"

DVDFile::DVDFile()
{
}

DVDFile::~DVDFile()
{
}

bool DVDFile::unload()
{
	return true;
}

bool DVDFile::load(const string& file)
{
	Assimp::Importer importer;
	name = file;
	scene = importer.ReadFile( file, aiProcess_CalcTangentSpace  | 
								aiProcess_Triangulate            |
								aiProcess_JoinIdenticalVertices  |
								aiProcess_SortByPType );
	if( !scene)
	{
		cout << "DVDFile::load(" << file << "): " << importer.GetErrorString();
		return false;
	}

	U32 index;
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
		getSubMeshes()[index]->getMaterial().texture = ResourceManager::getInstance().LoadResource<Texture2DFlipped>(pathName + path);
	}
	return true;
}
