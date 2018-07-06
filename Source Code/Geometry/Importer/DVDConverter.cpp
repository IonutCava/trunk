#include "Headers/DVDConverter.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "Core/Resources/Headers/ResourceCache.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Animations/Headers/AnimationUtils.h"

Mesh* DVDConverter::load(const std::string& file){	

	Mesh* tempMesh = New Mesh();
	_fileLocation = file;
	_modelName = _fileLocation.substr( _fileLocation.find_last_of( '/' ) + 1 );
	tempMesh->setName(_modelName);
	tempMesh->setResourceLocation(_fileLocation);
	_ppsteps = aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
			   aiProcess_FindInstances            | // search for instanced meshes and remove them by references to one master
			   aiProcess_OptimizeMeshes	          | // join small meshes, if possible;
			   aiProcess_OptimizeGraph            | // Nodes with no animations, bones, lights or cameras assigned are collapsed and joined.
			   aiProcessPreset_TargetRealtime_Quality |
			   0;

	bool removeLinesAndPoints = true;
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE , removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT : 0 );
	importer.SetPropertyInteger(AI_CONFIG_IMPORT_TER_MAKE_UVS , 1);
	importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);

	if(GFX_DEVICE.getApi() != OpenGL){
		_ppsteps |= aiProcess_ConvertToLeftHanded;
	}
	_aiScenePointer = importer.ReadFile( file, _ppsteps );


	if( !_aiScenePointer){
		ERROR_FN("DVDFile::load( %s ): %s", file.c_str(), importer.GetErrorString());
		return false;
	}
	for(U16 n = 0; n < _aiScenePointer->mNumMeshes; n++){
		
		//Skip points and lines ... for now -Ionut
		//ToDo: Fix this -Ionut
		if(_aiScenePointer->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_LINE || 
			_aiScenePointer->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_POINT ||
			_aiScenePointer->mMeshes[n]->mNumVertices == 0)
				continue;

		SubMesh* s = loadSubMeshGeometry(_aiScenePointer->mMeshes[n], n);

		if(s){
			if(s->getRefCount() == 1){
				Material* m = loadSubMeshMaterial(_aiScenePointer->mMaterials[_aiScenePointer->mMeshes[n]->mMaterialIndex],
												   std::string(s->getName()+ "_material"));
				s->setMaterial(m);
				m->setHardwareSkinning(s->_hasAnimations);
			}//else the Resource manager created a copy of the material
			tempMesh->addSubMesh(s->getName());
		}
			
	}
	
	tempMesh->setDrawState(true);
	return tempMesh;
}

SubMesh* DVDConverter::loadSubMeshGeometry(const aiMesh* source,U8 count){

	std::string temp;
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
	submeshdesc.setId(count);
	SubMesh* tempSubMesh = CreateResource<SubMesh>(submeshdesc);

	if(!tempSubMesh->getGeometryVBO()->getPosition().empty()){
		return tempSubMesh;
	}
	temp.clear();

	tempSubMesh->getGeometryVBO()->getPosition().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getNormal().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getTangent().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getBiTangent().reserve(source->mNumVertices);
	std::vector< std::vector<aiVertexWeight> >   weightsPerVertex(source->mNumVertices);

	if(source->HasBones()){
		tempSubMesh->getGeometryVBO()->getBoneIndices().reserve(source->mNumVertices);
		tempSubMesh->getGeometryVBO()->getBoneWeights().reserve(source->mNumVertices);	
		for(U32 a = 0; a < source->mNumBones; a++){
			const aiBone* bone = source->mBones[a];
			for( U32 b = 0; b < bone->mNumWeights; b++){
				weightsPerVertex[bone->mWeights[b].mVertexId].push_back(aiVertexWeight( a, bone->mWeights[b].mWeight));
			}
		}
	}
	  
	bool processTangents = true;
	if(!source->mTangents){
        processTangents = false;
		PRINT_FN("DVDConverter: SubMesh [ %s ] does not have tangent & biTangent data!", tempSubMesh->getName().c_str());
	}

	for(U32 j = 0; j < source->mNumVertices; j++){
			
		tempSubMesh->getGeometryVBO()->getPosition().push_back(vec3<F32>(source->mVertices[j].x,
															             source->mVertices[j].y,
																	     source->mVertices[j].z));
		tempSubMesh->getGeometryVBO()->getNormal().push_back(vec3<F32>(source->mNormals[j].x,
																       source->mNormals[j].y,
																       source->mNormals[j].z));
		if(processTangents){
			tempSubMesh->getGeometryVBO()->getTangent().push_back(vec3<F32>(source->mTangents[j].x,
																	        source->mTangents[j].y,
																	        source->mTangents[j].z));
			tempSubMesh->getGeometryVBO()->getBiTangent().push_back(vec3<F32>(source->mBitangents[j].x,
																	          source->mBitangents[j].y,
																		      source->mBitangents[j].z));
		}
		
		if( source->HasBones())	{
			vec4<U8>  boneIndices( 0, 0, 0, 0 );
			vec4<F32> boneWeights( 0, 0, 0, 0 );
			ai_assert( weightsPerVertex[j].size() <= 4);

			for( U8 a = 0; a < weightsPerVertex[j].size(); a++){

				boneIndices[a] = static_cast<U8>(weightsPerVertex[j][a].mVertexId);
				boneWeights[a] = weightsPerVertex[j][a].mWeight;

			}
			tempSubMesh->getGeometryVBO()->getBoneIndices().push_back(boneIndices);
			tempSubMesh->getGeometryVBO()->getBoneWeights().push_back(boneWeights);
		}



	}//endfor

	if(_aiScenePointer->HasAnimations() && source->HasBones()){
		// create animator from current scene and current submesh pointer in that scene
		tempSubMesh->createAnimatorFromScene(_aiScenePointer,count);
	}

	if(source->mTextureCoords[0] != NULL){
		tempSubMesh->getGeometryVBO()->getTexcoord().reserve(source->mNumVertices);
		for(U32 j = 0; j < source->mNumVertices; j++){
			tempSubMesh->getGeometryVBO()->getTexcoord().push_back(vec2<F32>(source->mTextureCoords[0][j].x,
																		     source->mTextureCoords[0][j].y));
		}//endfor
	}//endif

	U16 currentIndice = 0;
	U16 lowestInd = 0;
	U16 highestInd = 0;
	for(U32 k = 0; k < source->mNumFaces; k++){
		for(U32 m = 0; m < source->mFaces[k].mNumIndices; m++){
			currentIndice = source->mFaces[k].mIndices[m];
			if(currentIndice < lowestInd)  lowestInd  = currentIndice;
			if(currentIndice > highestInd) highestInd = currentIndice;

			tempSubMesh->getGeometryVBO()->getHWIndices().push_back(currentIndice);
		}
	}
	tempSubMesh->getIndiceLimits().x = lowestInd;
	tempSubMesh->getIndiceLimits().y = highestInd;

	return tempSubMesh;
		 
}

/// Load the material for the current SubMesh
Material* DVDConverter::loadSubMeshMaterial(const aiMaterial* source, const std::string& materialName) {

	/// See if the material already exists in a cooked state (XML file)
	Material* tempMaterial = XML::loadMaterial(materialName);
	if(tempMaterial) return tempMaterial;
	/// If it's not defined in an XML File, see if it was previously loaded by the Resource Cache
	bool skip = false;
	ResourceDescriptor tempMaterialDescriptor(materialName);
	if(FindResource(materialName)) skip = true;
	/// If we found it in the Resource Cache, return a copy of it
	tempMaterial = CreateResource<Material>(tempMaterialDescriptor);
	if(skip) return tempMaterial;

	ParamHandler& par = ParamHandler::getInstance();

	/// Compare load results with the standard succes value
	aiReturn result = AI_SUCCESS; 
	
	/// default diffuse color
	vec4<F32> tempColorVec4(0.8f, 0.8f, 0.8f,1.0f);

	/// Load diffuse color
	aiColor4D diffuse;
	if(AI_SUCCESS == aiGetMaterialColor(source,AI_MATKEY_COLOR_DIFFUSE, &diffuse)){
		tempColorVec4.r = diffuse.r;
		tempColorVec4.g = diffuse.g;
		tempColorVec4.b = diffuse.b;
		tempColorVec4.a = diffuse.a;
	}else{
		D_PRINT_FN("Material [ %s ] does not have a diffuse color", materialName.c_str());
	}
	tempMaterial->setDiffuse(tempColorVec4);
		
	/// default ambient color
	tempColorVec4.set(0.2f, 0.2f, 0.2f, 1.0f);

	/// Load ambient color
	aiColor4D ambient;
	if(AI_SUCCESS == aiGetMaterialColor(source,AI_MATKEY_COLOR_AMBIENT,&ambient)){
		tempColorVec4.r = ambient.r;
		tempColorVec4.g = ambient.g;
		tempColorVec4.b = ambient.b;
		tempColorVec4.a = ambient.a;
	}else{
		D_PRINT_FN("Material [ %s ] does not have an ambient color", materialName.c_str());
	}
	tempMaterial->setAmbient(tempColorVec4);
		
	/// default specular color
	tempColorVec4.set(1.0f,1.0f,1.0f,1.0f);

	// Load specular color
	aiColor4D specular;
	if(AI_SUCCESS == aiGetMaterialColor(source,AI_MATKEY_COLOR_SPECULAR,&specular)){
		tempColorVec4.r = specular.r;
		tempColorVec4.g = specular.g;
		tempColorVec4.b = specular.b;
		tempColorVec4.a = specular.a;
	}else{
		D_PRINT_FN("Material [ %s ] does not have a specular color", materialName.c_str());
	}
	tempMaterial->setSpecular(tempColorVec4);

	/// default emissive color
	tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);

	/// Load emissive color
	aiColor4D emissive;
	if(AI_SUCCESS == aiGetMaterialColor(source,AI_MATKEY_COLOR_EMISSIVE,&emissive)){
		tempColorVec4.r = emissive.r;
		tempColorVec4.g = emissive.g;
		tempColorVec4.b = emissive.b;
		tempColorVec4.b = emissive.a;
	}
	tempMaterial->setEmissive(tempColorVec4);

	/// default opacity value
	F32 opacity = 1.0f;
	/// Load material opacity value
	aiGetMaterialFloat(source,AI_MATKEY_OPACITY,&opacity);
	tempMaterial->setOpacityValue(opacity);


	/// default shading model
	I32 shadingModel = Material::SHADING_PHONG;
	/// Load shading model
	aiGetMaterialInteger(source,AI_MATKEY_SHADING_MODEL,&shadingModel);
	tempMaterial->setShadingMode(shadingModeInternal(shadingModel));

	/// Default shininess values
	F32 shininess = 15,strength = 1;
	/// Load shininess
	aiGetMaterialFloat(source,AI_MATKEY_SHININESS, &shininess);
	/// Load shininess strength
	aiGetMaterialFloat( source, AI_MATKEY_SHININESS_STRENGTH, &strength);
	tempMaterial->setShininess(shininess * strength);

	/// check if material is two sided
	I32 two_sided = 0;
	aiGetMaterialInteger(source, AI_MATKEY_TWOSIDED, &two_sided);
	tempMaterial->setDoubleSided(two_sided != 0);

	aiString tName; 
	aiTextureMapping mapping; 
	U32 uvInd;
	F32 blend; 
	aiTextureOp op;
	aiTextureMapMode mode[3];

	U8 count = 0;
	/// While we still have diffuse textures
	while(result == AI_SUCCESS){
		/// Load each one
		result = source->GetTexture(aiTextureType_DIFFUSE, count, 
									&tName, 
									&mapping, 
									&uvInd,
									&blend,
									&op, 
									mode);
		if(result != AI_SUCCESS) break;
		/// get full path
		std::string path = tName.data;
		/// get only image name
		std::string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		/// try to find a path name
		std::string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );
		/// look in default texture folder
		path = par.getParam<std::string>("assetsLocation")+"/"+par.getParam<std::string>("defaultTextureLocation") +"/"+ path;
		/// if we have a name and an extension
		if(!img_name.substr(img_name.find_first_of(".")).empty()){
			/// Is this the base texture or the secondary?
			Material::TextureUsage item;
			if(count == 0) item = Material::TEXTURE_BASE;
			else if(count == 1) item = Material::TEXTURE_SECOND;
			/// Load the texture resource
			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(item,textureRes,aiTextureOpToTextureOperation(op));
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif

		tName.Clear();
		count++;
		if(count == 2) break; //ToDo: Only 2 texture for now. Fix This! -Ionut;
	}//endwhile

	result = source->GetTexture(aiTextureType_NORMALS, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		std::string path = tName.data;
		std::string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		path = par.getParam<std::string>("assetsLocation")+"/"+par.getParam<std::string>("defaultTextureLocation") +"/"+ path;

		std::string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );
		if(img_name.rfind('.') !=  std::string::npos){

			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(Material::TEXTURE_NORMALMAP,textureRes,aiTextureOpToTextureOperation(op));
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif
	}//endif
	result = source->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		std::string path = tName.data;
		std::string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		path = par.getParam<std::string>("assetsLocation")+"/"+par.getParam<std::string>("defaultTextureLocation") +"/"+ path;
		std::string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );
		if(img_name.rfind('.') !=  std::string::npos){

			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(Material::TEXTURE_BUMP,textureRes,aiTextureOpToTextureOperation(op));
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif
	}//endif
	result = source->GetTexture(aiTextureType_OPACITY, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		std::string path = tName.data;
		std::string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		std::string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );

		path = par.getParam<std::string>("assetsLocation")+"/"+par.getParam<std::string>("defaultTextureLocation") +"/"+ path;
		
		if(img_name.rfind('.') !=  std::string::npos){
			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(Material::TEXTURE_OPACITY,textureRes,aiTextureOpToTextureOperation(op));
			tempMaterial->setDoubleSided(true);
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif
	}else{
		I32 flags = 0;
		aiGetMaterialInteger(source,AI_MATKEY_TEXFLAGS_DIFFUSE(0),&flags);

		// try to find out whether the diffuse texture has any
		// non-opaque pixels. If we find a few, use it as opacity texture
		if (tempMaterial->getTexture(Material::TEXTURE_BASE)){
			if(!(flags & aiTextureFlags_IgnoreAlpha) && 
				tempMaterial->getTexture(Material::TEXTURE_BASE)->hasTransparency()){
					Texture2D* textureRes = CreateResource<Texture2D>(ResourceDescriptor(tempMaterial->getTexture(Material::TEXTURE_BASE)->getName()));
					tempMaterial->setTexture(Material::TEXTURE_OPACITY,textureRes);
			}
		}
	}

	result = source->GetTexture(aiTextureType_SPECULAR, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		std::string path = tName.data;
		std::string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		std::string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );

		path = par.getParam<std::string>("assetsLocation")+"/"+par.getParam<std::string>("defaultTextureLocation") +"/"+ path;

		if(img_name.rfind('.') !=  std::string::npos){
			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(Material::TEXTURE_SPECULAR,textureRes,aiTextureOpToTextureOperation(op));
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif
	}//endif

	return tempMaterial;
}

Material::TextureOperation DVDConverter::aiTextureOpToTextureOperation(aiTextureOp op){
	switch(op){
		case aiTextureOp_Multiply:
			return Material::TextureOperation_Multiply;
		case aiTextureOp_Add:
			return Material::TextureOperation_Add;
		case aiTextureOp_Subtract:
			return Material::TextureOperation_Subtract;
		case aiTextureOp_Divide:
			return Material::TextureOperation_Divide;
		case aiTextureOp_SmoothAdd:
			return Material::TextureOperation_SmoothAdd;
		case aiTextureOp_SignedAdd:
			return Material::TextureOperation_SignedAdd;
		default:
			return Material::TextureOperation_Replace;
	};
}

Material::ShadingMode DVDConverter::shadingModeInternal(I32 mode){
	switch(mode){
		case aiShadingMode_Flat:
			return Material::SHADING_FLAT;
		case aiShadingMode_Gouraud:
			return Material::SHADING_GOURAUD;
		default:
		case aiShadingMode_Phong:
			return Material::SHADING_PHONG;
		case aiShadingMode_Blinn:
			return Material::SHADING_BLINN;
		case aiShadingMode_Toon:
			return Material::SHADING_TOON;
		case aiShadingMode_OrenNayar:
			return Material::SHADING_OREN_NAYAR;
		case aiShadingMode_Minnaert:
			return Material::SHADING_MINNAERT;
		case aiShadingMode_CookTorrance:
			return Material::SHADING_COOK_TORRANCE;
		case aiShadingMode_NoShading:
			return Material::SHADING_NONE;
		case aiShadingMode_Fresnel:
			return Material::SHADING_FRESNEL;
	};
}

Texture::TextureWrap DVDConverter::aiTextureMapModeToInternal(aiTextureMapMode mode){
	switch(mode){
		case aiTextureMapMode_Wrap:
			return Texture::TextureWrap_Wrap;
		case aiTextureMapMode_Clamp:
			return Texture::TextureWrap_Clamp;
		case aiTextureMapMode_Decal:
			return Texture::TextureWrap_Decal;
		default:
		case aiTextureMapMode_Mirror:
			return Texture::TextureWrap_Repeat;
	};
}