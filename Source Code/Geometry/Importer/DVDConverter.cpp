#include "Headers/DVDConverter.h"

#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "Core/Resources/Headers/ResourceCache.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Geometry/Shapes/Headers/SkinnedMesh.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Geometry/Animations/Headers/AnimationUtils.h"


DVDConverter::DVDConverter() : _loadcount(0), _init(false){

}

DVDConverter::~DVDConverter(){
	SAFE_DELETE(importer);
}

bool DVDConverter::init(){
	if(_init) {
		ERROR_FN(Locale::get("ERROR_DOUBLE_IMPORTER_INIT"));
		return false;
	}
	importer = New Assimp::Importer();
	
	bool removeLinesAndPoints = true;
	importer->SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE , removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT : 0 );
	importer->SetPropertyInteger(AI_CONFIG_IMPORT_TER_MAKE_UVS , 1);
	importer->SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
	_ppsteps = aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
			   aiProcess_FindInstances            | // search for instanced meshes and remove them by references to one master
			   aiProcess_OptimizeMeshes	          | // join small meshes, if possible;
			   aiProcess_OptimizeGraph            | // Nodes with no animations, bones, lights or cameras assigned are collapsed and joined.
			   aiProcessPreset_TargetRealtime_Quality |
			   0;

	if(GFX_DEVICE.getApi() != OpenGL){
		_ppsteps |= aiProcess_ConvertToLeftHanded;
	}
	_init = true;
	return true;
}

Mesh* DVDConverter::load(const std::string& file){	
	if(!_init){
		ERROR_FN(Locale::get("ERROR_NO_INIT_IMPORTER_LOAD"));
		return NULL;
	}
	_fileLocation = file;
	_modelName = _fileLocation.substr( _fileLocation.find_last_of( '/' ) + 1 );
	
	_aiScenePointer = importer->ReadFile( file, _ppsteps );

	if( !_aiScenePointer){
		ERROR_FN(Locale::get("ERROR_IMORTER_FILE"), file.c_str(), importer->GetErrorString());
		return false;
	}
	Mesh* tempMesh = NULL;
	for(U16 n = 0; n < _aiScenePointer->mNumMeshes; n++){
		
		//Skip points and lines ... for now -Ionut
#pragma message("ToDo: Fix skipping points and lines on geometry import -Ionut")
		if(_aiScenePointer->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_LINE || 
			_aiScenePointer->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_POINT ||
			_aiScenePointer->mMeshes[n]->mNumVertices == 0)
				continue;

		

		if(SubMesh* s = loadSubMeshGeometry(_aiScenePointer->mMeshes[n], n)){
			bool skinnedSubMesh = (s->getFlag() == Object3D::OBJECT_FLAG_SKINNED ? true : false);
			if(!tempMesh){
				tempMesh  = (skinnedSubMesh ? New SkinnedMesh() : New Mesh());
				tempMesh->setName(_modelName);
				tempMesh->setResourceLocation(_fileLocation);
			}

			if(s->getRefCount() == 1){
				Material* m = loadSubMeshMaterial(_aiScenePointer->mMaterials[_aiScenePointer->mMeshes[n]->mMaterialIndex],
												   std::string(s->getName()+ "_material"));
				s->setMaterial(m);
				m->setHardwareSkinning(skinnedSubMesh);
			}//else the Resource manager created a copy of the material
			tempMesh->addSubMesh(s->getName());
		}
			
	}
	assert(tempMesh != NULL);
	tempMesh->getSceneNodeRenderState().setDrawState(true);
	_loadcount++; //< increment counter
	return tempMesh;
}

///If we are loading a LOD variant of a submesh, find it first, append LOD geometry,
///but do not return the mesh again as it will be duplicated in the Mesh parent object
///instead, return NULL and mesh and material creation for this instance will be skipped.
SubMesh* DVDConverter::loadSubMeshGeometry(const aiMesh* source,U8 count){
	///VERY IMPORTANT: default submesh, LOD0 should always be created first!!
	///an assert is added in the LODn, where n >= 1, loading code to make sure the LOD0 submesh exists first
	bool baseMeshLoading = false;

	std::string temp(source->mName.C_Str());

	if(temp.empty()){
		std::stringstream ss;
		ss << _fileLocation.substr(_fileLocation.rfind("/")+1,_fileLocation.length());
		ss << "-submesh-";
		ss << (U32)count;
		temp = ss.str();
	}

	SubMesh* tempSubMesh = NULL;
	bool skinned = source->HasBones();
	if(temp.find(".LOD1") != std::string::npos || 
		temp.find(".LOD2") != std::string::npos/* ||
		temp.find(".LODn") != std::string::npos*/){ ///Add as many LOD levels as you please
		tempSubMesh = dynamic_cast<SubMesh* >(FindResource(_fileLocation.substr(0,_fileLocation.rfind(".LOD"))));
		assert(tempSubMesh != NULL);
		tempSubMesh->incLODcount();
	}else{
		//Submesh is created as a resource when added to the scenegraph
		ResourceDescriptor submeshdesc(temp);
		submeshdesc.setFlag(true);
		submeshdesc.setId(count);
		if(skinned) 
			submeshdesc.setEnumValue(Object3D::OBJECT_FLAG_SKINNED); 
		tempSubMesh = CreateResource<SubMesh>(submeshdesc);
		///it may be already loaded
		if(!tempSubMesh->getGeometryVBO()->getPosition().empty()){
			return tempSubMesh;
		}
		baseMeshLoading = true;
	}

	tempSubMesh->getGeometryVBO()->reservePositionCount(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getNormal().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getTangent().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getBiTangent().reserve(source->mNumVertices);
	vectorImpl< vectorImpl<vertexWeight> >   weightsPerVertex(source->mNumVertices);

	if(skinned){
		tempSubMesh->getGeometryVBO()->getBoneIndices().reserve(source->mNumVertices);
		tempSubMesh->getGeometryVBO()->getBoneWeights().reserve(source->mNumVertices);	
		assert(source->mNumBones < 256); ///<Fit in U8
		for(U8 a = 0; a < source->mNumBones; a++){
			const aiBone* bone = source->mBones[a];
			for( U32 b = 0; b < bone->mNumWeights; b++){
				weightsPerVertex[bone->mWeights[b].mVertexId].push_back(vertexWeight( a, bone->mWeights[b].mWeight));
			}
		}
	}
	  
	bool processTangents = true;
	if(!source->mTangents){
        processTangents = false;
		D_PRINT_FN(Locale::get("SUBMESH_NO_TANGENT"), tempSubMesh->getName().c_str());
	}


	for(U32 j = 0; j < source->mNumVertices; j++){
		tempSubMesh->getGeometryVBO()->addPosition(vec3<F32>(source->mVertices[j].x,
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
		
		if( skinned )	{
			vec4<U8>  boneIndices( 0, 0, 0, 0 );
			vec4<F32> boneWeights( 0, 0, 0, 0 );
			assert( weightsPerVertex[j].size() <= 4);

			for( U8 a = 0; a < weightsPerVertex[j].size(); a++){

				boneIndices[a] = weightsPerVertex[j][a]._boneId;
				boneWeights[a] = weightsPerVertex[j][a]._boneWeight;

			}
			tempSubMesh->getGeometryVBO()->getBoneIndices().push_back(boneIndices);
			tempSubMesh->getGeometryVBO()->getBoneWeights().push_back(boneWeights);
		}



	}//endfor
	if(_aiScenePointer->HasAnimations() && skinned){
		// create animator from current scene and current submesh pointer in that scene
		dynamic_cast<SkinnedSubMesh*>(tempSubMesh)->createAnimatorFromScene(_aiScenePointer,count);
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
		U32 indiceCount = source->mFaces[k].mNumIndices;
		if(indiceCount == 3){
			vec3<U32> triangleTemp(source->mFaces[k].mIndices[0],source->mFaces[k].mIndices[1],source->mFaces[k].mIndices[2]);
			tempSubMesh->getGeometryVBO()->getTriangles().push_back(triangleTemp);
		}

		for(U32 m = 0; m < indiceCount; m++){
			currentIndice = source->mFaces[k].mIndices[m];
			if(currentIndice < lowestInd)  lowestInd  = currentIndice;
			if(currentIndice > highestInd) highestInd = currentIndice;

			tempSubMesh->getGeometryVBO()->getHWIndices().push_back(currentIndice);
		}

	}
	tempSubMesh->getGeometryVBO()->setIndiceLimits(vec2<U16>(lowestInd,highestInd), tempSubMesh->getLODcount() - 1);

	if(baseMeshLoading)
		return tempSubMesh;
	else
		return NULL;
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
		D_PRINT_FN(Locale::get("MATERIAL_NO_DIFFUSE"), materialName.c_str());
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
		D_PRINT_FN(Locale::get("MATERIAL_NO_AMBIENT"), materialName.c_str());
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
		D_PRINT_FN(Locale::get("MATERIAL_NO_SPECULAR"), materialName.c_str());
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
	F32 finalValue = shininess * strength;
	CLAMP<F32>(finalValue,0.0f,127.5f);
	tempMaterial->setShininess(finalValue);

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
			Material::TextureUsage item = Material::TEXTURE_BASE;
			if(count == 1) item = Material::TEXTURE_SECOND;
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
#pragma message("ToDo: Use more than 2 textures for each material. Fix This! -Ionut")
		if(count == 2) break; 
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
			tempMaterial->setBumpMethod(Material::BUMP_NORMAL);
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
			tempMaterial->setBumpMethod(Material::BUMP_NORMAL);
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

TextureWrap DVDConverter::aiTextureMapModeToInternal(aiTextureMapMode mode){
	switch(mode){
		case aiTextureMapMode_Wrap:
			return TEXTURE_REPEAT;
		case aiTextureMapMode_Clamp:
			return TEXTURE_CLAMP;
		case aiTextureMapMode_Decal:
			return TEXTURE_DECAL;
		default:
		case aiTextureMapMode_Mirror:
			return TEXTURE_REPEAT;
	};
}