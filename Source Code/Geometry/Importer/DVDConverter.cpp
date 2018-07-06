#include "Headers/DVDConverter.h"

#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "Geometry/Animations/Headers/AnimationUtils.h"

namespace Divide {

namespace {
size_t GetMatchingFormat(const Assimp::Exporter& exporter,
                         const stringImpl& extension) {
    for (size_t i = 0, end = exporter.GetExportFormatCount(); i < end; ++i) {
        const aiExportFormatDesc* const e =
            exporter.GetExportFormatDescription(i);
        if (extension == e->fileExtension) {
            return i;
        }
    }
    return SIZE_MAX;
}
};

DVDConverter::DVDConverter() : _ppsteps(0) {
    aiTextureMapModeTable[aiTextureMapMode_Wrap] = TextureWrap::CLAMP;
    aiTextureMapModeTable[aiTextureMapMode_Clamp] =
        TextureWrap::CLAMP_TO_EDGE;
    aiTextureMapModeTable[aiTextureMapMode_Decal] = TextureWrap::DECAL;
    aiTextureMapModeTable[aiTextureMapMode_Mirror] =
        TextureWrap::REPEAT;
    aiShadingModeInternalTable[aiShadingMode_Fresnel] =
        Material::ShadingMode::COOK_TORRANCE;
    aiShadingModeInternalTable[aiShadingMode_NoShading] =
        Material::ShadingMode::FLAT;
    aiShadingModeInternalTable[aiShadingMode_CookTorrance] =
        Material::ShadingMode::COOK_TORRANCE;
    aiShadingModeInternalTable[aiShadingMode_Minnaert] =
        Material::ShadingMode::OREN_NAYAR;
    aiShadingModeInternalTable[aiShadingMode_OrenNayar] =
        Material::ShadingMode::OREN_NAYAR;
    aiShadingModeInternalTable[aiShadingMode_Toon] =
        Material::ShadingMode::TOON;
    aiShadingModeInternalTable[aiShadingMode_Blinn] =
        Material::ShadingMode::BLINN_PHONG;
    aiShadingModeInternalTable[aiShadingMode_Phong] =
        Material::ShadingMode::PHONG;
    aiShadingModeInternalTable[aiShadingMode_Gouraud] =
        Material::ShadingMode::BLINN_PHONG;
    aiShadingModeInternalTable[aiShadingMode_Flat] =
        Material::ShadingMode::FLAT;
    aiTextureOperationTable[aiTextureOp_Multiply] =
        Material::TextureOperation::MULTIPLY;
    aiTextureOperationTable[aiTextureOp_Add] =
        Material::TextureOperation::ADD;
    aiTextureOperationTable[aiTextureOp_Subtract] =
        Material::TextureOperation::SUBTRACT;
    aiTextureOperationTable[aiTextureOp_Divide] =
        Material::TextureOperation::DIVIDE;
    aiTextureOperationTable[aiTextureOp_SmoothAdd] =
        Material::TextureOperation::SMOOTH_ADD;
    aiTextureOperationTable[aiTextureOp_SignedAdd] =
        Material::TextureOperation::SIGNED_ADD;
    aiTextureOperationTable[aiTextureOp_SignedAdd] =
        Material::TextureOperation::SIGNED_ADD;
    aiTextureOperationTable[/*aiTextureOp_Replace*/ 7] =
        Material::TextureOperation::REPLACE;

    importer = MemoryManager_NEW Assimp::Importer();
}

DVDConverter::~DVDConverter() {
    MemoryManager::DELETE(importer);
}

bool DVDConverter::init() {
    if (_ppsteps != 0) {
        Console::errorfn(Locale::get("ERROR_DOUBLE_IMPORTER_INIT"));
        return false;
    }

    bool removeLinesAndPoints = true;
    importer->SetPropertyInteger(
        AI_CONFIG_PP_SBP_REMOVE,
        removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT
                             : 0);
    importer->SetPropertyInteger(AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
    importer->SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
    _ppsteps =
        aiProcess_TransformUVCoords |  // preprocess UV transformations
                                       // (scaling, translation ...)
        aiProcess_FindInstances |      // search for instanced meshes and remove
                                       // them by references to one master
        aiProcess_OptimizeMeshes |     // join small meshes, if possible;
        aiProcess_OptimizeGraph |      // Nodes without
                                       // animations/bones/lights/cameras are
                                       // collapsed & joined.
        aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality |
        aiProcess_LimitBoneWeights | aiProcess_RemoveRedundantMaterials |
        aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_SortByPType |
        aiProcess_FindDegenerates | aiProcess_FindInvalidData | 0;

    if (GFX_DEVICE.getAPI() != GFXDevice::RenderAPI::OpenGL &&
        GFX_DEVICE.getAPI() != GFXDevice::RenderAPI::OpenGLES) {
        _ppsteps |= aiProcess_ConvertToLeftHanded;
    }

    return _ppsteps != 0;
}

Mesh* DVDConverter::load(const stringImpl& file) {
    if (_ppsteps == 0) {
        Console::errorfn(Locale::get("ERROR_NO_INIT_IMPORTER_LOAD"));
        return nullptr;
    }
    D32 start = 0.0;
    D32 elapsed = 0.0;

    _fileLocation = file;

    _modelName = _fileLocation.substr(_fileLocation.find_last_of('/') + 1);
    stringImpl processedModel =
        _fileLocation.substr(0, _fileLocation.find_last_of("/")) + "/bin/" +
        _modelName;
    /*FILE* fp = fopen(processedModel.c_str(), "rb");
    if (fp || false){
        fclose(fp);
        start = Time::ElapsedMilliseconds();
        _aiScenePointer = importer->ReadFile(processedModel, 0);
        elapsed = Time::ElapsedMilliseconds() - start;
    }else{*/
    start = Time::ElapsedMilliseconds();
    _aiScenePointer = importer->ReadFile(file.c_str(), _ppsteps);
    elapsed = Time::ElapsedMilliseconds() - start;

    /* Assimp::Exporter exporter;

     size_t i = GetMatchingFormat(exporter,
 _modelName.substr(_modelName.find_last_of(".") + 1));

     if (i != SIZE_MAX) {
         exporter.Export(_aiScenePointer,
 exporter.GetExportFormatDescription(i)->id, processedModel);
     }
 }*/

    Console::d_printfn(Locale::get("LOAD_MESH_TIME"), _modelName.c_str(),
                       Time::MillisecondsToSeconds(elapsed));

    if (!_aiScenePointer) {
        Console::errorfn(Locale::get("ERROR_IMPORTER_FILE"), file.c_str(),
                         importer->GetErrorString());
        return nullptr;
    }
    start = Time::ElapsedMilliseconds(true);
    Mesh* tempMesh =
        MemoryManager_NEW Mesh(_aiScenePointer->HasAnimations()
                                   ? Object3D::ObjectFlag::OBJECT_FLAG_SKINNED
                                   : Object3D::ObjectFlag::OBJECT_FLAG_NONE);
    tempMesh->setName(_modelName);
    tempMesh->setResourceLocation(_fileLocation);

    SubMesh* tempSubMesh = nullptr;
    U32 vertCount = 0;
    bool skinned = false;
    for (U16 n = 0; n < _aiScenePointer->mNumMeshes; n++) {
        vertCount += _aiScenePointer->mMeshes[n]->mNumVertices;
        if (_aiScenePointer->mMeshes[n]->HasBones()) {
            skinned = true;
        }
    }

    VertexBuffer* vb = tempMesh->getGeometryVB();
    vb->useLargeIndices(vertCount + 1 > std::numeric_limits<U16>::max());
    vb->reservePositionCount(vertCount);
    vb->reserveNormalCount(vertCount);
    vb->reserveTangentCount(vertCount);
    vb->getTexcoord().reserve(vertCount);
    if (skinned) {
        vb->getBoneIndices().reserve(vertCount);
        vb->getBoneWeights().reserve(vertCount);
        // create animator from current scene and current submesh pointer in
        // that scene
        tempMesh->initAnimator(_aiScenePointer);
    }

    U8 submeshBoneOffset = 0;
    for (U16 n = 0; n < _aiScenePointer->mNumMeshes; n++) {
        aiMesh* currentMesh = _aiScenePointer->mMeshes[n];
        // Skip points and lines ... for now -Ionut
        if (currentMesh->mNumVertices == 0) {
            continue;
        }
        tempSubMesh = loadSubMeshGeometry(currentMesh, tempMesh, n, submeshBoneOffset);

        if (tempSubMesh) {
            if (!tempSubMesh->getMaterialTpl()) {
                tempSubMesh->setMaterialTpl(loadSubMeshMaterial(
                    skinned,
                    _aiScenePointer->mMaterials[currentMesh->mMaterialIndex],
                    stringImpl(tempSubMesh->getName() + "_material")));
            }

            tempMesh->addSubMesh(tempSubMesh);
        }
    }

    assert(tempMesh != nullptr);

    tempMesh->renderState().setDrawState(true);
    tempMesh->getGeometryVB()->Create();

    elapsed = Time::ElapsedMilliseconds(true) - start;
    Console::d_printfn(Locale::get("PARSE_MESH_TIME"), _modelName.c_str(),
                       Time::MillisecondsToSeconds(elapsed));

    return tempMesh;
}

/// If we are loading a LOD variant of a submesh, find it first, append LOD
/// geometry,
/// but do not return the mesh again as it will be duplicated in the Mesh parent
/// object
/// instead, return nullptr and mesh and material creation for this instance
/// will be skipped.
SubMesh* DVDConverter::loadSubMeshGeometry(const aiMesh* source,
                                           Mesh* parentMesh,
                                           U16 count, 
                                           U8& submeshBoneOffsetOut) {
    /// VERY IMPORTANT: default submesh, LOD0 should always be created first!!
    /// an assert is added in the LODn, where n >= 1, loading code to make sure
    /// the LOD0 submesh exists first
    bool baseMeshLoading = false;

    stringImpl temp(source->mName.C_Str());

    if (temp.empty()) {
        std::stringstream ss;
        ss << _fileLocation.substr(_fileLocation.rfind("/") + 1,
                                   _fileLocation.length()).c_str();
        ss << "-submesh-";
        ss << to_uint(count);
        temp = stringAlg::toBase(ss.str());
    }

    SubMesh* tempSubMesh = nullptr;
    bool skinned = source->HasBones();
    if (temp.find(".LOD1") != stringImpl::npos ||
        temp.find(".LOD2") !=
            stringImpl::npos /* ||
        temp.find(".LODn") != stringImpl::npos*/) {  /// Add as many LOD levels as you please
        tempSubMesh = FindResourceImpl<SubMesh>(
            _fileLocation.substr(0, _fileLocation.rfind(".LOD")));
        assert(tempSubMesh != nullptr);
        tempSubMesh->incLODcount();
    } else {
        // Submesh is created as a resource when added to the scenegraph
        ResourceDescriptor submeshdesc(temp);
        submeshdesc.setFlag(true);
        submeshdesc.setID(count);
        if (skinned) {
            submeshdesc.setEnumValue(
                to_uint(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
        }
        tempSubMesh = CreateResource<SubMesh>(submeshdesc);
        if (!tempSubMesh) {
            return nullptr;
        }
        // it may be already loaded
        if (tempSubMesh->getParentMesh()) {
            return tempSubMesh;
        }
        baseMeshLoading = true;
    }

    vectorImpl<vectorImpl<vertexWeight> > weightsPerVertex(
        source->mNumVertices);

    if (skinned) {
        assert(source->mNumBones < 256);  ///<Fit in U8
        for (U8 a = 0; a < source->mNumBones; a++) {
            const aiBone* bone = source->mBones[a];
            for (U32 b = 0; b < bone->mNumWeights; b++) {
                weightsPerVertex[bone->mWeights[b].mVertexId].push_back(
                    vertexWeight(a, bone->mWeights[b].mWeight));
            }
        }
    }

    bool processTangents = true;
    if (!source->mTangents) {
        processTangents = false;
        Console::d_printfn(Locale::get("SUBMESH_NO_TANGENT"),
                           tempSubMesh->getName().c_str());
    }

    BoundingBox importBB;

    VertexBuffer* vb = parentMesh->getGeometryVB();
    U32 previousOffset = to_uint(vb->getPosition().size());

    vec3<F32> position, normal, tangent;
    P32 boneIndices;
    vec4<F32> boneWeights;

    for (U32 j = 0; j < source->mNumVertices; j++) {
        position.setV(&source->mVertices[j].x);
        normal.setV(&source->mNormals[j].x);

        vb->addPosition(position);
        vb->addNormal(normal);
        importBB.Add(position);
        if (processTangents) {
            tangent.setV(&source->mTangents[j].x);
            vb->addTangent(tangent);
        }

        if (skinned) {
            boneIndices.i = 0;
            boneWeights.reset();

            assert(weightsPerVertex[j].size() <= 4);

            for (U8 a = 0; a < weightsPerVertex[j].size(); a++) {
                U16 boneID = weightsPerVertex[j][a]._boneID + submeshBoneOffsetOut;
                assert(boneID < std::numeric_limits<U8>::max());
                boneIndices.b[a] = static_cast<U8>(boneID);
                boneWeights[a] = weightsPerVertex[j][a]._boneWeight;
            }

            vb->addBoneIndex(boneIndices);
            vb->addBoneWeight(boneWeights);
        }
    }  // endfor

    submeshBoneOffsetOut += static_cast<U8>(source->mNumBones);
    Attorney::SubMeshDVDConverter::setGeometryLimits(
        *tempSubMesh, importBB.getMin(), importBB.getMax());

    if (source->mTextureCoords[0] != nullptr) {
        vec2<F32> texCoord;
        for (U32 j = 0; j < source->mNumVertices; j++) {
            texCoord.set(source->mTextureCoords[0][j].x,
                         source->mTextureCoords[0][j].y);
            vb->addTexCoord(texCoord);
        }  // endfor
    }      // endif

    U32 idxCount = 0;
    U32 currentIndice = 0;
    U32 lowestInd = std::numeric_limits<U32>::max();
    U32 highestInd = 0;
    vec3<U32> triangleTemp;

    for (U32 k = 0; k < source->mNumFaces; k++) {
        assert(source->mFaces[k].mNumIndices == 3);

        for (U32 m = 0; m < 3; m++) {
            currentIndice = source->mFaces[k].mIndices[m] + previousOffset;
            vb->addIndex(currentIndice);
            idxCount++;
            triangleTemp[m] = currentIndice;

            lowestInd = std::min(currentIndice, lowestInd);
            highestInd = std::max(currentIndice, highestInd);
        }

        tempSubMesh->addTriangle(triangleTemp);
    }

    tempSubMesh->setGeometryPartitionID(vb->partitionBuffer(idxCount));
    vb->shrinkAllDataToFit();

    return baseMeshLoading ? tempSubMesh : nullptr;
}

/// Load the material for the current SubMesh
Material* DVDConverter::loadSubMeshMaterial(bool skinned,
                                            const aiMaterial* source,
                                            const stringImpl& materialName) {
    // See if the material already exists in a cooked state (XML file)
    STUBBED("LOADING MATERIALS FROM XML IS DISABLED FOR NOW! - Ionut")
#if !defined(DEBUG)
    const bool DISABLE_MAT_FROM_FILE = true;
#else
    const bool DISABLE_MAT_FROM_FILE = true;
#endif
    Material* tempMaterial = nullptr;
    if (!DISABLE_MAT_FROM_FILE) {
        tempMaterial = XML::loadMaterial(materialName.c_str());
        if (tempMaterial) {
            return tempMaterial;
        }
    }

    // If it's not defined in an XML File, see if it was previously loaded by
    // the Resource Cache
    bool skip = (FindResourceImpl<Material>(materialName) != nullptr);

    // If we found it in the Resource Cache, return a copy of it
    ResourceDescriptor materialDesc(materialName);
    if (skinned) {
        materialDesc.setEnumValue(
            to_uint(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
    }

    tempMaterial = CreateResource<Material>(materialDesc);

    if (skip) {
        return tempMaterial;
    }

    // Compare load results with the standard success value
    aiReturn result = AI_SUCCESS;

    // default diffuse color
    vec4<F32> tempColorVec4(0.8f, 0.8f, 0.8f, 1.0f);

    // Load diffuse color
    aiColor4D diffuse;
    if (AI_SUCCESS ==
        aiGetMaterialColor(source, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
        tempColorVec4.setV(&diffuse.r);
    } else {
        Console::d_printfn(Locale::get("MATERIAL_NO_DIFFUSE"),
                           materialName.c_str());
    }

    tempMaterial->setDiffuse(tempColorVec4);

    // default ambient color
    tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);

    // Load ambient color
    aiColor4D ambient;
    if (AI_SUCCESS ==
        aiGetMaterialColor(source, AI_MATKEY_COLOR_AMBIENT, &ambient)) {
        tempColorVec4.setV(&ambient.r);
    } else {
        Console::d_printfn(Locale::get("MATERIAL_NO_AMBIENT"),
                           materialName.c_str());
    }
    tempMaterial->setAmbient(tempColorVec4);

    // default specular color
    tempColorVec4.set(1.0f, 1.0f, 1.0f, 1.0f);

    // Load specular color
    aiColor4D specular;
    if (AI_SUCCESS ==
        aiGetMaterialColor(source, AI_MATKEY_COLOR_SPECULAR, &specular)) {
        tempColorVec4.setV(&specular.r);
    } else {
        Console::d_printfn(Locale::get("MATERIAL_NO_SPECULAR"),
                           materialName.c_str());
    }
    tempMaterial->setSpecular(tempColorVec4);

    // default emissive color
    tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);

    // Load emissive color
    aiColor4D emissive;
    if (AI_SUCCESS ==
        aiGetMaterialColor(source, AI_MATKEY_COLOR_EMISSIVE, &emissive)) {
        tempColorVec4.setV(&emissive.r);
    }
    tempMaterial->setEmissive(tempColorVec4);

    // default opacity value
    F32 opacity = 1.0f;
    // Load material opacity value
    aiGetMaterialFloat(source, AI_MATKEY_OPACITY, &opacity);
    tempMaterial->setOpacity(opacity);

    // default shading model
    I32 shadingModel = to_int(Material::ShadingMode::PHONG);
    // Load shading model
    aiGetMaterialInteger(source, AI_MATKEY_SHADING_MODEL, &shadingModel);
    tempMaterial->setShadingMode(aiShadingModeInternalTable[shadingModel]);

    // Default shininess values
    F32 shininess = 15, strength = 1;
    // Load shininess
    aiGetMaterialFloat(source, AI_MATKEY_SHININESS, &shininess);
    // Load shininess strength
    aiGetMaterialFloat(source, AI_MATKEY_SHININESS_STRENGTH, &strength);
    F32 finalValue = shininess * strength;
    CLAMP<F32>(finalValue, 0.0f, 127.5f);
    tempMaterial->setShininess(finalValue);

    // check if material is two sided
    I32 two_sided = 0;
    aiGetMaterialInteger(source, AI_MATKEY_TWOSIDED, &two_sided);
    tempMaterial->setDoubleSided(two_sided != 0);

    aiString tName;
    aiTextureMapping mapping;
    U32 uvInd;
    F32 blend;
    aiTextureOp op = aiTextureOp_Multiply;
    aiTextureMapMode mode[3] = {_aiTextureMapMode_Force32Bit,
                                _aiTextureMapMode_Force32Bit,
                                _aiTextureMapMode_Force32Bit};

    ParamHandler& par = ParamHandler::getInstance();

    SamplerDescriptor textureSampler;
    U8 count = 0;
    // While we still have diffuse textures
    while (result == AI_SUCCESS) {
        // Load each one
        result = source->GetTexture(aiTextureType_DIFFUSE, count, &tName,
                                    &mapping, &uvInd, &blend, &op, mode);
        if (result != AI_SUCCESS) {
            break;
        }
        // get full path
        stringImpl path = tName.data;
        // get only image name
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);
        // try to find a path name
        stringImpl pathName =
            _fileLocation.substr(0, _fileLocation.rfind("/") + 1);
        // look in default texture folder
        path = par.getParam<stringImpl>("assetsLocation") + "/" +
               par.getParam<stringImpl>("defaultTextureLocation") + "/" + path;
        // if we have a name and an extension
        if (!img_name.substr(img_name.find_first_of(".")).empty()) {
            // Load the texture resource
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                textureSampler.setWrapMode(aiTextureMapModeTable[mode[0]],
                                           aiTextureMapModeTable[mode[1]],
                                           aiTextureMapModeTable[mode[2]]);
            }
            textureSampler.toggleSRGBColorSpace(true);
            ResourceDescriptor texture(img_name);
            texture.setResourceLocation(path);
            texture.setFlag(true);
            texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);
            Texture* textureRes = CreateResource<Texture>(texture);
            assert(tempMaterial != nullptr);
            assert(textureRes != nullptr);
            // The first texture is always "Replace"
            tempMaterial->setTexture(
                count == 1 ? ShaderProgram::TextureUsage::UNIT1
                           : ShaderProgram::TextureUsage::UNIT0,
                textureRes,
                count == 0
                    ? Material::TextureOperation::REPLACE
                    : aiTextureOperationTable[op]);
        }  // endif

        tName.Clear();
        count++;
        if (count == 2) {
            break;
        }
        STUBBED(
            "ToDo: Use more than 2 textures for each material. Fix This! "
            "-Ionut")
    }  // endwhile

    result = source->GetTexture(aiTextureType_NORMALS, 0, &tName, &mapping,
                                &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        stringImpl path = tName.data;
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);
        path = par.getParam<stringImpl>("assetsLocation") + "/" +
               par.getParam<stringImpl>("defaultTextureLocation") + "/" + path;

        stringImpl pathName =
            _fileLocation.substr(0, _fileLocation.rfind("/") + 1);
        if (img_name.rfind('.') != stringImpl::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                textureSampler.setWrapMode(aiTextureMapModeTable[mode[0]],
                                           aiTextureMapModeTable[mode[1]],
                                           aiTextureMapModeTable[mode[2]]);
            }
            textureSampler.toggleSRGBColorSpace(false);
            ResourceDescriptor texture(img_name);
            texture.setResourceLocation(path);
            texture.setFlag(true);
            texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);
            Texture* textureRes = CreateResource<Texture>(texture);
            tempMaterial->setTexture(ShaderProgram::TextureUsage::NORMALMAP,
                                     textureRes, aiTextureOperationTable[op]);
            tempMaterial->setBumpMethod(Material::BumpMethod::NORMAL);
        }  // endif
    }      // endif

    result = source->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping,
                                &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        stringImpl path = tName.data;
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);
        path = par.getParam<stringImpl>("assetsLocation") + "/" +
               par.getParam<stringImpl>("defaultTextureLocation") + "/" + path;
        stringImpl pathName =
            _fileLocation.substr(0, _fileLocation.rfind("/") + 1);
        if (img_name.rfind('.') != stringImpl::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                textureSampler.setWrapMode(aiTextureMapModeTable[mode[0]],
                                           aiTextureMapModeTable[mode[1]],
                                           aiTextureMapModeTable[mode[2]]);
            }
            textureSampler.toggleSRGBColorSpace(false);
            ResourceDescriptor texture(img_name);
            texture.setResourceLocation(path);
            texture.setFlag(true);
            texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);
            Texture* textureRes = CreateResource<Texture>(texture);
            tempMaterial->setTexture(ShaderProgram::TextureUsage::NORMALMAP,
                                     textureRes, aiTextureOperationTable[op]);
            tempMaterial->setBumpMethod(Material::BumpMethod::NORMAL);
        }  // endif
    }      // endif
    result = source->GetTexture(aiTextureType_OPACITY, 0, &tName, &mapping,
                                &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        stringImpl path = tName.data;
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);
        stringImpl pathName =
            _fileLocation.substr(0, _fileLocation.rfind("/") + 1);

        path = par.getParam<stringImpl>("assetsLocation") + "/" +
               par.getParam<stringImpl>("defaultTextureLocation") + "/" + path;

        if (img_name.rfind('.') != stringImpl::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                textureSampler.setWrapMode(aiTextureMapModeTable[mode[0]],
                                           aiTextureMapModeTable[mode[1]],
                                           aiTextureMapModeTable[mode[2]]);
            }
            textureSampler.toggleSRGBColorSpace(false);
            ResourceDescriptor texture(img_name);
            texture.setResourceLocation(path);
            texture.setFlag(true);
            texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);
            Texture* textureRes = CreateResource<Texture>(texture);
            tempMaterial->setTexture(ShaderProgram::TextureUsage::OPACITY, textureRes,
                                     aiTextureOperationTable[op]);
            tempMaterial->setDoubleSided(true);
        }  // endif
    } else {
        I32 flags = 0;
        aiGetMaterialInteger(source, AI_MATKEY_TEXFLAGS_DIFFUSE(0), &flags);

        // try to find out whether the diffuse texture has any
        // non-opaque pixels. If we find a few, use it as opacity texture
        if (tempMaterial->getTexture(ShaderProgram::TextureUsage::UNIT0)) {
            if (!(flags & aiTextureFlags_IgnoreAlpha) &&
                tempMaterial->getTexture(ShaderProgram::TextureUsage::UNIT0)
                    ->hasTransparency()) {
                ResourceDescriptor texDesc(
                    tempMaterial->getTexture(ShaderProgram::TextureUsage::UNIT0)
                        ->getName());
                Texture* textureRes = CreateResource<Texture>(texDesc);
                tempMaterial->setTexture(ShaderProgram::TextureUsage::OPACITY,
                                         textureRes);
            }
        }
    }

    result = source->GetTexture(aiTextureType_SPECULAR, 0, &tName, &mapping,
                                &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        stringImpl path = tName.data;
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);
        stringImpl pathName =
            _fileLocation.substr(0, _fileLocation.rfind("/") + 1);

        path = par.getParam<stringImpl>("assetsLocation") + "/" +
               par.getParam<stringImpl>("defaultTextureLocation") + "/" + path;

        if (img_name.rfind('.') != stringImpl::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                textureSampler.setWrapMode(aiTextureMapModeTable[mode[0]],
                                           aiTextureMapModeTable[mode[1]],
                                           aiTextureMapModeTable[mode[2]]);
            }
            textureSampler.toggleSRGBColorSpace(false);
            ResourceDescriptor texture(img_name);
            texture.setResourceLocation(path);
            texture.setFlag(true);
            texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);
            Texture* textureRes = CreateResource<Texture>(texture);
            tempMaterial->setTexture(ShaderProgram::TextureUsage::SPECULAR,
                                     textureRes, aiTextureOperationTable[op]);
        }  // endif
    }      // endif

    return tempMaterial;
}
};