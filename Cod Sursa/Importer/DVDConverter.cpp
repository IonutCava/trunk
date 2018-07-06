#include "DVDConverter.h"

#include <assimp.hpp>      // C++ importer interface
#include <aiPostProcess.h> // Post processing flags
#include "Managers/ResourceManager.h"


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
}

bool DVDFile::unload()
{
	ResourceManager::getInstance().remove(getName());
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

	if(!getShaders().empty())
	{
		for(U8 i = 0; i < getShaders().size(); i++)
		{
			ResourceManager::getInstance().remove(getShaders()[i]->getName());
			delete getShaders()[i];
		}
		getShaders().clear();
	}

	else return false;
	return true;
}

bool DVDFile::clean()
{
	if(_shouldDelete)	return unload();
	else return false;
}

bool DVDFile::load(const string& file)
{		
	bool removeLinesAndPoints = true;
	getName() = file;
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE , removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT : 0 );

	if(GFXDevice::getInstance().getApi() > 6)
		_ppsteps |= aiProcess_ConvertToLeftHanded;

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
		//Skip points and lines ... for now -Ionut
		//ToDo: Fix this -Ionut
		if(scene->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_LINE || 
			scene->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_POINT )
				continue;

		string temp;
		char a;
		for(U32 j = 0; j < scene->mMeshes[n]->mName.length; j++)
		{
			a = scene->mMeshes[n]->mName.data[j];
			temp += a;
		}

		index = getSubMeshes().size();
		addSubMesh(new SubMesh(temp));
		temp.clear();

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
		  
		getSubMeshes()[index]->getGeometryVBO()->Create();
	 
	
		aiReturn result = AI_SUCCESS; 
		aiString tName; aiTextureMapping mapping; unsigned int uvInd;
		float blend;  aiTextureOp op; aiTextureMapMode mode[3];

		aiMaterial *mat = scene->mMaterials[scene->mMeshes[n]->mMaterialIndex];

		aiColor3D tempColor;	F32 alpha = 1.0f;
		vec4 tempColorVec4(0.8f, 0.8f, 0.8f,1.0f);

		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE,tempColor))
		{
			tempColorVec4.r = tempColor.r;tempColorVec4.g = tempColor.g;tempColorVec4.b = tempColor.b;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_TRANSPARENT,alpha))
			tempColorVec4.a = alpha;

		getSubMeshes()[index]->getMaterial().diffuse = tempColorVec4;
		
		tempColorVec4.set(0.2f, 0.2f, 0.2f, 1.0f);
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_AMBIENT,tempColor))
		{
			tempColorVec4.r = tempColor.r;tempColorVec4.g = tempColor.g;tempColorVec4.b = tempColor.b;
		}
		getSubMeshes()[index]->getMaterial().ambient  = tempColorVec4;
		
		tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_SPECULAR,tempColor))
		{
			tempColorVec4.r = tempColor.r;tempColorVec4.g = tempColor.g;tempColorVec4.b = tempColor.b;
		}

		getSubMeshes()[index]->getMaterial().specular = tempColorVec4;

		tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_EMISSIVE,tempColor))
		{
			tempColorVec4.r = tempColor.r;tempColorVec4.g = tempColor.g;tempColorVec4.b = tempColor.b;
		}
		getSubMeshes()[index]->getMaterial().emmissive = tempColorVec4;


		F32 shininess = 0,strength = 0;
		U32 max = 1;

		aiReturn ret1 = aiGetMaterialFloatArray( mat, AI_MATKEY_SHININESS, &shininess, &max);
		max = 1;
		aiReturn ret2 = aiGetMaterialFloatArray( mat, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
		if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
			 getSubMeshes()[index]->getMaterial().shininess = shininess * strength;
		else
				getSubMeshes()[index]->getMaterial().shininess = 0.0f;

		while(result == AI_SUCCESS)
		{
			result = mat->GetTexture(aiTextureType_DIFFUSE, getSubMeshes()[index]->getMaterial().textures.size(), &tName, &mapping, &uvInd, &blend, &op, mode);
			if(result != AI_SUCCESS) break;
			string path = tName.data;
			string img_name = path.substr( path.find_last_of( '/' ) + 1 );
			if(!img_name.substr(img_name.find_first_of(".")).empty()) 
			{
				string pathName = file.substr( 0, file.rfind("/")+1 );
				getSubMeshes()[index]->getMaterial().textures.push_back(ResourceManager::getInstance().LoadResource<Texture2D>(pathName + "../texturi/"  + img_name,true));
			}
			tName.Clear();
		}

		if(!getSubMeshes()[index]->getMaterial().bumpMap)
		{
			result = mat->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
			if(result == AI_SUCCESS)
			{
				string path = tName.data;
				string img_name = path.substr( path.find_last_of( '/' ) + 1 );
				if(img_name.rfind('.') !=  string::npos)
				{
					string pathName = file.substr( 0, file.rfind("/")+1 );
					getSubMeshes()[index]->getMaterial().bumpMap = ResourceManager::getInstance().LoadResource<Texture2D>(pathName + "../texturi/"  + img_name,true);
				}
			}
		}
	}

	_render = true;
	return _render;
}
