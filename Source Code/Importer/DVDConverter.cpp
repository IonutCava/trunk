#include "DVDConverter.h"

#include <aiScene.h> 
#include <assimp.hpp>      // C++ importer interface
#include <aiPostProcess.h> // Post processing flags
#include "Managers/ResourceManager.h"
using namespace std;

Mesh* DVDConverter::load(const string& file)
{	
	Mesh* tempMesh = new Mesh();
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
	bool removeLinesAndPoints = true;
	tempMesh->setName(file);
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
		Con::getInstance().errorfn("DVDFile::load( %s ): %s", file.c_str(), importer.GetErrorString());
		return false;
	}

	U16 index=0;
	for(U16 n = 0; n < scene->mNumMeshes; n++)
	{
		
		//Skip points and lines ... for now -Ionut
		//ToDo: Fix this -Ionut
		if(scene->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_LINE || 
			scene->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_POINT )
				continue;
		if(scene->mMeshes[n]->mNumVertices == 0) continue; 
		string temp;
		char a;
		for(U16 j = 0; j < scene->mMeshes[n]->mName.length; j++)
		{
			a = scene->mMeshes[n]->mName.data[j];
			temp += a;
		}
		
		if(temp.empty()){
			char number[3];
			sprintf(number,"%d",n);
			temp = file+"-submesh-"+number;
		}
		index = tempMesh->getSubMeshes().size();
		tempMesh->addSubMesh(new SubMesh(temp));
		temp.clear();
		tempMesh->getSubMeshes()[index]->getGeometryVBO()->getPosition().reserve(scene->mMeshes[n]->mNumVertices);
		tempMesh->getSubMeshes()[index]->getGeometryVBO()->getNormal().reserve(scene->mMeshes[n]->mNumVertices);
		std::vector< std::vector<vertexWeight> >   weightsPerVertex(scene->mMeshes[n]->mNumVertices);
/*
		if(scene->mMeshes[n]->HasBones())
		{
		
			for(U16 b = 0; b < scene->mMeshes[n]->mNumBones; b++)
			{
				const aiBone* bone = scene->mMeshes[n]->mBones[b];
				for( U16 bi = 0; bi < bone->mNumWeights; bi++)
					weightsPerVertex[bone->mWeights[bi].mVertexId].push_back(vertexWeight( b, bone->mWeights[bi].mWeight));
			}
		}
*/
		for(U32 j = 0; j < scene->mMeshes[n]->mNumVertices; j++)
		{
			
			tempMesh->getSubMeshes()[index]->getGeometryVBO()->getPosition().push_back(vec3(scene->mMeshes[n]->mVertices[j].x,
																				  scene->mMeshes[n]->mVertices[j].y,
																				  scene->mMeshes[n]->mVertices[j].z));
			tempMesh->getSubMeshes()[index]->getGeometryVBO()->getNormal().push_back(vec3(scene->mMeshes[n]->mNormals[j].x,
																				scene->mMeshes[n]->mNormals[j].y,
																				scene->mMeshes[n]->mNormals[j].z));
			/*vector<U8> boneIndices, boneWeights;
			boneIndices.push_back(0);boneWeights.push_back(0);
			boneIndices.push_back(0);boneWeights.push_back(0);
			boneIndices.push_back(0);boneWeights.push_back(0);
			boneIndices.push_back(0);boneWeights.push_back(0);

			if( scene->mMeshes[n]->HasBones())	{
				ai_assert( weightsPerVertex[x].size() <= 4);
				for( U8 a = 0; a < weightsPerVertex[j].size(); a++)
				{
					boneIndices.push_back(weightsPerVertex[j][a]._vertexId);
					boneWeights.push_back((U8) (weightsPerVertex[j][a]._weight * 255.0f));
				}
			}

			tempMesh->getSubMeshes()[index]->getGeometryVBO()->getBoneIndices().push_back(boneIndices);
			tempMesh->getSubMeshes()[index]->getGeometryVBO()->getBoneWeights().push_back(boneWeights);
			*/
		}

		if(scene->mMeshes[n]->mTextureCoords[0] != NULL)
		{
			tempMesh->getSubMeshes()[index]->getGeometryVBO()->getTexcoord().reserve(scene->mMeshes[n]->mNumVertices);
			tempMesh->getSubMeshes()[index]->getGeometryVBO()->getTangent().reserve(scene->mMeshes[n]->mNumVertices);
			for(U32 j = 0; j < scene->mMeshes[n]->mNumVertices; j++)
			{
				tempMesh->getSubMeshes()[index]->getGeometryVBO()->getTexcoord().push_back(vec2(scene->mMeshes[n]->mTextureCoords[0][j].x,
																					  scene->mMeshes[n]->mTextureCoords[0][j].y));
				tempMesh->getSubMeshes()[index]->getGeometryVBO()->getTangent().push_back(vec3(scene->mMeshes[n]->mTangents[j].x,
																					   scene->mMeshes[n]->mTangents[j].y,
																					   scene->mMeshes[n]->mTangents[j].z));
			}
		}
		for(U32 k = 0; k < scene->mMeshes[n]->mNumFaces; k++)
				for(U32 m = 0; m < scene->mMeshes[n]->mFaces[k].mNumIndices; m++)
					tempMesh->getSubMeshes()[index]->getIndices().push_back(scene->mMeshes[n]->mFaces[k].mIndices[m]);
		 
		
	
		tempMesh->getSubMeshes()[index]->getGeometryVBO()->Create();
	 
	
		aiReturn result = AI_SUCCESS; 
		aiString tName; aiTextureMapping mapping; U32 uvInd;
		F32 blend;  aiTextureOp op; aiTextureMapMode mode[3];

		aiMaterial *mat = scene->mMaterials[scene->mMeshes[n]->mMaterialIndex];

		aiColor3D tempColor;	F32 alpha = 1.0f;
		vec4 tempColorVec4(0.8f, 0.8f, 0.8f,1.0f);

		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE,tempColor))
		{
			tempColorVec4.r = tempColor.r;tempColorVec4.g = tempColor.g;tempColorVec4.b = tempColor.b;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_TRANSPARENT,alpha))
			tempColorVec4.a = alpha;

		tempMesh->getSubMeshes()[index]->getMaterial().diffuse = tempColorVec4;
		
		tempColorVec4.set(0.2f, 0.2f, 0.2f, 1.0f);
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_AMBIENT,tempColor))
		{
			tempColorVec4.r = tempColor.r;tempColorVec4.g = tempColor.g;tempColorVec4.b = tempColor.b;
		}
		tempMesh->getSubMeshes()[index]->getMaterial().ambient  = tempColorVec4;
		
		tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_SPECULAR,tempColor))
		{
			tempColorVec4.r = tempColor.r;tempColorVec4.g = tempColor.g;tempColorVec4.b = tempColor.b;
		}

		tempMesh->getSubMeshes()[index]->getMaterial().specular = tempColorVec4;

		tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_EMISSIVE,tempColor))
		{
			tempColorVec4.r = tempColor.r;tempColorVec4.g = tempColor.g;tempColorVec4.b = tempColor.b;
		}
		tempMesh->getSubMeshes()[index]->getMaterial().emmissive = tempColorVec4;


		F32 shininess = 0,strength = 0;
		U32 max = 1;

		aiReturn ret1 = aiGetMaterialFloatArray( mat, AI_MATKEY_SHININESS, &shininess, &max);
		max = 1;
		aiReturn ret2 = aiGetMaterialFloatArray( mat, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
		if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
			tempMesh->getSubMeshes()[index]->getMaterial().shininess = shininess * strength;
		else
			tempMesh->getSubMeshes()[index]->getMaterial().shininess = 0.0f;

		U8 count = 0;
		while(result == AI_SUCCESS){
			result = mat->GetTexture(aiTextureType_DIFFUSE, count, &tName, &mapping, &uvInd, &blend, &op, mode);
			if(result != AI_SUCCESS) break;
			string path = tName.data;
			string img_name = path.substr( path.find_last_of( '/' ) + 1 );
			if(!img_name.substr(img_name.find_first_of(".")).empty()){
				string pathName = file.substr( 0, file.rfind("/")+1 );
				Material::TextureUsage item;
				if(count == 0) item = Material::TEXTURE_BASE;
				else if(count == 1) item = Material::TEXTURE_SECOND;
				tempMesh->getSubMeshes()[index]->getMaterial().setTexture(item,ResourceManager::getInstance().LoadResource<Texture2D>(pathName + "../texturi/"  + img_name,true));
			}
			tName.Clear();
			count++;
			if(count == 2) break; //ToDo: Only 2 texture for now. Fix This! -Ionut;
		}

		result = mat->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
		if(result == AI_SUCCESS){
			string path = tName.data;
			string img_name = path.substr( path.find_last_of( '/' ) + 1 );
			if(img_name.rfind('.') !=  string::npos){
					string pathName = file.substr( 0, file.rfind("/")+1 );
					tempMesh->getSubMeshes()[index]->getMaterial().setTexture(Material::TEXTURE_BUMP,ResourceManager::getInstance().LoadResource<Texture2D>(pathName + "../texturi/"  + img_name,true));
				}
			}
		

	}
	tempMesh->setVisibility(true);
	return tempMesh;
}

