#include "DVDConverter.h"

#include <aiScene.h> 
#include <assimp.hpp>      // C++ importer interface
#include <aiPostProcess.h> // Post processing flags
#include "Managers/ResourceManager.h"
#include "Utility/Headers/XMLParser.h"

using namespace std;

Mesh* DVDConverter::load(const string& file){	
	Mesh* tempMesh = New Mesh();
	_fileLocation = file;
	_modelName = _fileLocation.substr( _fileLocation.find_last_of( '/' ) + 1 );
	tempMesh->setName(_modelName);
	tempMesh->setResourceLocation(_fileLocation);
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
			   aiProcess_OptimizeGraph            | // Nodes with no animations, bones, lights or cameras assigned are collapsed and joined.
			   0;
	bool removeLinesAndPoints = true;
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE , removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT : 0 );

	if(GFXDevice::getInstance().getApi() > 6)
		_ppsteps |= aiProcess_ConvertToLeftHanded;

	scene = importer.ReadFile( file, _ppsteps                    |
								aiProcess_Triangulate            |
								aiProcess_SplitLargeMeshes	     |
								aiProcess_SortByPType            |
								aiProcess_GenSmoothNormals);

	if( !scene){
		Console::getInstance().errorfn("DVDFile::load( %s ): %s", file.c_str(), importer.GetErrorString());
		return false;
	}

	for(U16 n = 0; n < scene->mNumMeshes; n++){
		
		//Skip points and lines ... for now -Ionut
		//ToDo: Fix this -Ionut
		if(scene->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_LINE || 
			scene->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_POINT )
				continue;
		if(scene->mMeshes[n]->mNumVertices == 0) continue; 
		SubMesh* s = loadSubMeshGeometry(scene->mMeshes[n], n);
		Material* m = loadSubMeshMaterial(scene->mMaterials[scene->mMeshes[n]->mMaterialIndex],string(s->getName()+ "_material"));
		if(s && m) {
			s->getGeometryVBO()->Create();
			s->setMaterial(m);
			tempMesh->addSubMesh(s->getName());
		}
			
	}
	
	tempMesh->setRenderState(true);
	return tempMesh;
}

SubMesh* DVDConverter::loadSubMeshGeometry(aiMesh* source,U8 count){
	string temp;
	bool skip = false;
	char a;
	for(U16 j = 0; j < source->mName.length; j++){
		a = source->mName.data[j];
		temp += a;
	}
	
	if(temp.empty()){
		char number[3];
		sprintf(number,"%d",count);
		temp = _fileLocation.substr(_fileLocation.rfind("/")+1,_fileLocation.length())+"-submesh-"+number;
	}

	//Submesh is created as a resource when added to the scenegraph
	ResourceDescriptor submeshdesc(temp);
	submeshdesc.setFlag(true);
	SubMesh* tempSubMesh = ResourceManager::getInstance().loadResource<SubMesh>(submeshdesc);

	if(!tempSubMesh->getGeometryVBO()->getPosition().empty()){
		Console::getInstance().printfn("SubMesh geometry [ %s ] already exists. Skipping creation ...",temp.c_str());
		return tempSubMesh;
	}
	temp.clear();

	tempSubMesh->getGeometryVBO()->getPosition().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getNormal().reserve(source->mNumVertices);
	std::vector< std::vector<vertexWeight> >   weightsPerVertex(source->mNumVertices);
/*
	if(source->HasBones()){
	
		for(U16 b = 0; b < source->mNumBones; b++){
			const aiBone* bone = source->mBones[b];
			for( U16 bi = 0; bi < bone->mNumWeights; bi++)
				weightsPerVertex[bone->mWeights[bi].mVertexId].push_back(vertexWeight( b, bone->mWeights[bi].mWeight));
		}
	}
*/
	for(U32 j = 0; j < source->mNumVertices; j++){
			
		tempSubMesh->getGeometryVBO()->getPosition().push_back(vec3(source->mVertices[j].x,
															        source->mVertices[j].y,
																	source->mVertices[j].z));
		tempSubMesh->getGeometryVBO()->getNormal().push_back(vec3(source->mNormals[j].x,
																  source->mNormals[j].y,
																  source->mNormals[j].z));
/*		vector<U8> boneIndices, boneWeights;
		boneIndices.push_back(0);boneWeights.push_back(0);
		boneIndices.push_back(0);boneWeights.push_back(0);
		boneIndices.push_back(0);boneWeights.push_back(0);
		boneIndices.push_back(0);boneWeights.push_back(0);

		if( source->HasBones())	{
			ai_assert( weightsPerVertex[x].size() <= 4);
			for( U8 a = 0; a < weightsPerVertex[j].size(); a++){
				boneIndices.push_back(weightsPerVertex[j][a]._vertexId);
				boneWeights.push_back((U8) (weightsPerVertex[j][a]._weight * 255.0f));
			}
		}

		tempSubMesh->getGeometryVBO()->getBoneIndices().push_back(boneIndices);
		tempSubMesh->getGeometryVBO()->getBoneWeights().push_back(boneWeights);
*/
	}//endfor

	if(source->mTextureCoords[0] != NULL){
		tempSubMesh->getGeometryVBO()->getTexcoord().reserve(source->mNumVertices);
		tempSubMesh->getGeometryVBO()->getTangent().reserve(source->mNumVertices);
		for(U32 j = 0; j < source->mNumVertices; j++){
			tempSubMesh->getGeometryVBO()->getTexcoord().push_back(vec2(source->mTextureCoords[0][j].x,
																		source->mTextureCoords[0][j].y));
			tempSubMesh->getGeometryVBO()->getTangent().push_back(vec3(source->mTangents[j].x,
																	   source->mTangents[j].y,
																	   source->mTangents[j].z));
		}//endfor
	}//endif

	for(U32 k = 0; k < source->mNumFaces; k++)
			for(U32 m = 0; m < source->mFaces[k].mNumIndices; m++)
				tempSubMesh->getIndices().push_back(source->mFaces[k].mIndices[m]);

	return tempSubMesh;
		 
}

Material* DVDConverter::loadSubMeshMaterial(aiMaterial* source, const string& materialName){

	Material* tempMaterial = XML::loadMaterial(materialName);
	if(tempMaterial) return tempMaterial;
	bool skip = false;
	ResourceDescriptor tempMaterialDescriptor(materialName);
	if(ResourceManager::getInstance().find(materialName)) skip = true;
	tempMaterial = ResourceManager::getInstance().loadResource<Material>(tempMaterialDescriptor);

	if(skip) return tempMaterial;

	aiReturn result = AI_SUCCESS; 
	aiString tName; aiTextureMapping mapping; U32 uvInd;
	F32 blend;  aiTextureOp op; aiTextureMapMode mode[3];

	aiColor3D tempColor;	
	F32 alpha = 1.0f;
	vec4 tempColorVec4(0.8f, 0.8f, 0.8f,1.0f);

	if(AI_SUCCESS == source->Get(AI_MATKEY_COLOR_DIFFUSE,tempColor)){
		tempColorVec4.r = tempColor.r;
		tempColorVec4.g = tempColor.g;
		tempColorVec4.b = tempColor.b;
		if(AI_SUCCESS == source->Get(AI_MATKEY_COLOR_TRANSPARENT,alpha))
			tempColorVec4.a = alpha;
	}
	tempMaterial->setDiffuse(tempColorVec4);
		
	tempColorVec4.set(0.2f, 0.2f, 0.2f, 1.0f);
	if(AI_SUCCESS == source->Get(AI_MATKEY_COLOR_AMBIENT,tempColor)){
		tempColorVec4.r = tempColor.r;
		tempColorVec4.g = tempColor.g;
		tempColorVec4.b = tempColor.b;
	}
	tempMaterial->setAmbient(tempColorVec4);
		
	tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);
	if(AI_SUCCESS == source->Get(AI_MATKEY_COLOR_SPECULAR,tempColor)){
		tempColorVec4.r = tempColor.r;
		tempColorVec4.g = tempColor.g;
		tempColorVec4.b = tempColor.b;
	}
	tempMaterial->setSpecular(tempColorVec4);

	tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);
	if(AI_SUCCESS == source->Get(AI_MATKEY_COLOR_EMISSIVE,tempColor)){
		tempColorVec4.r = tempColor.r;
		tempColorVec4.g = tempColor.g;
		tempColorVec4.b = tempColor.b;
	}
	tempMaterial->setEmissive(tempColorVec4);


	F32 shininess = 0,strength = 0;
	U32 max = 1;

	aiReturn ret1 = aiGetMaterialFloatArray( source, AI_MATKEY_SHININESS, &shininess, &max);
	max = 1;
	aiReturn ret2 = aiGetMaterialFloatArray( source, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
	if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
		tempMaterial->setShininess(shininess * strength);
	else
		tempMaterial->setShininess(50.0f);

	U8 count = 0;
	while(result == AI_SUCCESS){
		result = source->GetTexture(aiTextureType_DIFFUSE, count, &tName, &mapping, &uvInd, &blend, &op, mode);
		if(result != AI_SUCCESS) break;
		string path = tName.data;
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );

		if(!img_name.substr(img_name.find_first_of(".")).empty()){
			string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );

			Material::TextureUsage item;
			if(count == 0) item = Material::TEXTURE_BASE;
			else if(count == 1) item = Material::TEXTURE_SECOND;

			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(pathName + "../texturi/"+img_name);
			texture.setFlag(true);
			tempMaterial->setTexture(item,ResourceManager::getInstance().loadResource<Texture2D>(texture));
		}//endif

		tName.Clear();
		count++;
		if(count == 2) break; //ToDo: Only 2 texture for now. Fix This! -Ionut;
	}//endwhile

	result = source->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		string path = tName.data;
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		if(img_name.rfind('.') !=  string::npos){
			string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );
			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(pathName + "../texturi/"+img_name);
			texture.setFlag(true);
			tempMaterial->setTexture(Material::TEXTURE_BUMP,ResourceManager::getInstance().loadResource<Texture2D>(texture));
		}//endif
	}//endif

	result = source->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		string path = tName.data;
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		if(img_name.rfind('.') !=  string::npos){
			string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );
			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(pathName + "../texturi/"+img_name);
			texture.setFlag(true);
			tempMaterial->setTexture(Material::TEXTURE_BUMP,ResourceManager::getInstance().loadResource<Texture2D>(texture));
		}//endif
	}//endif
	max = 1;
	I32 two_sided = 0;
	if((result == aiGetMaterialIntegerArray(source, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided){
		tempMaterial->setTwoSided(true);
	}else{
		tempMaterial->setTwoSided(false);
	}

	return tempMaterial;
}