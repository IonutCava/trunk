#include "config.h"

#include "Headers/DVDConverter.h"
#include "Headers/MeshImporter.h"

#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "Geometry/Animations/Headers/AnimationUtils.h"

namespace Divide {
    namespace {
        static const bool g_removeLinesAndPoints = true;

        struct vertexWeight {
            U8 _boneID;
            F32 _boneWeight;
            vertexWeight(U8 ID, F32 weight) : _boneID(ID), _boneWeight(weight) {}
        };

        /// Recursively creates an internal node structure matching the current scene and animation.
        Bone* createBoneTree(aiNode* pNode, Bone* parent) {
            Bone* internalNode = MemoryManager_NEW Bone(pNode->mName.data);
            // set the parent; in case this is the root node, it will be null
            internalNode->_parent = parent;

            internalNode->_localTransform = pNode->mTransformation;
            if (!Config::USE_OPENGL_RENDERING) {
                internalNode->_localTransform.Transpose();
            }

            internalNode->_originalLocalTransform = internalNode->_localTransform;
            calculateBoneToWorldTransform(internalNode);

            // continue for all child nodes and assign the created internal nodes as our
            // children recursively call this function on all children
            for (U32 i = 0; i < pNode->mNumChildren; ++i) {
                internalNode->_children.push_back(
                    createBoneTree(pNode->mChildren[i], internalNode));
            }

            return internalNode;
        }

    };

    hashMapImpl<U32, TextureWrap> DVDConverter::fillTextureWrapMap() {
        hashMapImpl<U32, TextureWrap> wrapMap;
        wrapMap[aiTextureMapMode_Wrap] = TextureWrap::CLAMP;
        wrapMap[aiTextureMapMode_Clamp] = TextureWrap::CLAMP_TO_EDGE;
        wrapMap[aiTextureMapMode_Mirror] = TextureWrap::REPEAT;
        wrapMap[aiTextureMapMode_Decal] = TextureWrap::DECAL;
        return wrapMap;
    }

    hashMapImpl<U32, Material::ShadingMode> DVDConverter::fillShadingModeMap() {
        hashMapImpl<U32, Material::ShadingMode> shadingMap;
        shadingMap[aiShadingMode_Fresnel] = Material::ShadingMode::COOK_TORRANCE;
        shadingMap[aiShadingMode_NoShading] = Material::ShadingMode::FLAT;
        shadingMap[aiShadingMode_CookTorrance] = Material::ShadingMode::COOK_TORRANCE;
        shadingMap[aiShadingMode_Minnaert] = Material::ShadingMode::OREN_NAYAR;
        shadingMap[aiShadingMode_OrenNayar] = Material::ShadingMode::OREN_NAYAR;
        shadingMap[aiShadingMode_Toon] = Material::ShadingMode::TOON;
        shadingMap[aiShadingMode_Blinn] = Material::ShadingMode::BLINN_PHONG;
        shadingMap[aiShadingMode_Phong] = Material::ShadingMode::PHONG;
        shadingMap[aiShadingMode_Gouraud] = Material::ShadingMode::BLINN_PHONG;
        shadingMap[aiShadingMode_Flat] = Material::ShadingMode::FLAT;
        return shadingMap;
    }

    hashMapImpl<U32, Material::TextureOperation> DVDConverter::fillTextureOperationMap() {
        hashMapImpl<U32, Material::TextureOperation> operationMap;
        operationMap[aiTextureOp_Multiply] = Material::TextureOperation::MULTIPLY;
        operationMap[aiTextureOp_Add] = Material::TextureOperation::ADD;
        operationMap[aiTextureOp_Subtract] = Material::TextureOperation::SUBTRACT;
        operationMap[aiTextureOp_Divide] = Material::TextureOperation::DIVIDE;
        operationMap[aiTextureOp_SmoothAdd] = Material::TextureOperation::SMOOTH_ADD;
        operationMap[aiTextureOp_SignedAdd] = Material::TextureOperation::SIGNED_ADD;
        operationMap[/*aiTextureOp_Replace*/ 7] = Material::TextureOperation::REPLACE;
        return operationMap;
    }


    hashMapImpl<U32, TextureWrap>
    DVDConverter::aiTextureMapModeTable = DVDConverter::fillTextureWrapMap();
    hashMapImpl<U32, Material::ShadingMode>
    DVDConverter::aiShadingModeInternalTable = DVDConverter::fillShadingModeMap();
    hashMapImpl<U32, Material::TextureOperation>
    DVDConverter::aiTextureOperationTable = DVDConverter::fillTextureOperationMap();

DVDConverter::DVDConverter()
{
}

DVDConverter::DVDConverter(Import::ImportData& target, const stringImpl& file, bool& result) {
    result = load(target, file);
}

DVDConverter::~DVDConverter()
{
}

bool DVDConverter::load(Import::ImportData& target, const stringImpl& file) {
    Assimp::Importer importer;

    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
                                g_removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT
                                                       : 0);

    importer.SetPropertyInteger(AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);

    U32 ppsteps =
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

    if (!Config::USE_OPENGL_RENDERING) {
        ppsteps |= aiProcess_ConvertToLeftHanded;
    }

    const aiScene* aiScenePointer = importer.ReadFile(file.c_str(), ppsteps);

    if (!aiScenePointer) {
        Console::errorfn(Locale::get(_ID("ERROR_IMPORTER_FILE")), file.c_str(),
                         importer.GetErrorString());
        return nullptr;
    }

    target._hasAnimations = aiScenePointer->HasAnimations();
    if (target._hasAnimations) {
        target._skeleton = createBoneTree(aiScenePointer->mRootNode, nullptr);
        target._bones.reserve(to_int(target._skeleton->hierarchyDepth()));

        for (U16 meshPointer = 0; meshPointer < aiScenePointer->mNumMeshes; ++meshPointer) {
            const aiMesh* mesh = aiScenePointer->mMeshes[meshPointer];
            for (U32 n = 0; n < mesh->mNumBones; ++n) {
                const aiBone* bone = mesh->mBones[n];

                Bone* found = target._skeleton->find(bone->mName.data);
                assert(found != nullptr);

                found->_offsetMatrix = bone->mOffsetMatrix;
                target._bones.push_back(found);
            }
        }

        for (size_t i(0); i < aiScenePointer->mNumAnimations; i++) {
            aiAnimation* animation = aiScenePointer->mAnimations[i];
            if (animation->mDuration > 0.0f) {
                target._animations.push_back(std::make_shared<AnimEvaluator>(animation, to_uint(i)));
            }
        }
    }
    
    U32 vertCount = 0;
    for (U16 n = 0; n < aiScenePointer->mNumMeshes; ++n) {
        vertCount += aiScenePointer->mMeshes[n]->mNumVertices;
    }

    target._vertexBuffer = GFX_DEVICE.newVB();
    target._vertexBuffer->useLargeIndices(vertCount + 1 > std::numeric_limits<U16>::max());
    target._vertexBuffer->setVertexCount(vertCount);

    U8 submeshBoneOffset = 0;
    U32 previousOffset = 0;
    U32 numMeshes = aiScenePointer->mNumMeshes;
    target._subMeshData.reserve(numMeshes);

    for (U16 n = 0; n < numMeshes; ++n) {
        aiMesh* currentMesh = aiScenePointer->mMeshes[n];
        // Skip points and lines ... for now -Ionut
        if (currentMesh->mNumVertices == 0) {
            continue;
        }

        Import::SubMeshData subMeshTemp;

        subMeshTemp._name = currentMesh->mName.C_Str();
        subMeshTemp._index = to_uint(n);
        if (subMeshTemp._name.empty()) {
            subMeshTemp._name = Util::StringFormat("%s-submesh-%d", 
                                                   file.substr(file.rfind("/") + 1, file.length()).c_str(),
                                                   n);
        }
       
        subMeshTemp._boneCount = currentMesh->mNumBones;
        loadSubMeshGeometry(currentMesh, 
                            target._vertexBuffer,
                            subMeshTemp,
                            submeshBoneOffset,
                            previousOffset);

        loadSubMeshMaterial(subMeshTemp._material,
                            aiScenePointer->mMaterials[currentMesh->mMaterialIndex],
                            subMeshTemp._name + "_material",
                            file,
                            subMeshTemp._boneCount > 0);
                            

        target._subMeshData.push_back(subMeshTemp);
    }

    return true;
}

void DVDConverter::loadSubMeshGeometry(const aiMesh* source,
                                       VertexBuffer* targetBuffer,
                                       Import::SubMeshData& subMeshData,
                                       U8& submeshBoneOffsetOut,
                                       U32& previousVertOffset) {
    BoundingBox importBB;
    U32 previousOffset = previousVertOffset;
    previousVertOffset += source->mNumVertices;

    for (U32 j = 0; j < source->mNumVertices; ++j) {
        U32 idx = j + previousOffset;
        targetBuffer->modifyPositionValue(idx, source->mVertices[j].x,
                                               source->mVertices[j].y,
                                               source->mVertices[j].z);

        targetBuffer->modifyNormalValue(idx, source->mNormals[j].x,
                                             source->mNormals[j].y,
                                             source->mNormals[j].z);

        importBB.add(targetBuffer->getPosition(idx));
    }
    
    subMeshData._minPos = importBB.getMin();
    subMeshData._maxPos = importBB.getMax();

    if (source->mTextureCoords[0] != nullptr) {
        for (U32 j = 0; j < source->mNumVertices; ++j) {
            targetBuffer->modifyTexCoordValue(j + previousOffset,
                                             source->mTextureCoords[0][j].x,
                                             source->mTextureCoords[0][j].y);
        }
    }

    if (source->mTangents != nullptr) {
        for (U32 j = 0; j < source->mNumVertices; ++j) {
            targetBuffer->modifyTangentValue(j + previousOffset,
                                             source->mTangents[j].x,
                                             source->mTangents[j].y,
                                             source->mTangents[j].z);
        }
    } else {
        Console::d_printfn(Locale::get(_ID("SUBMESH_NO_TANGENT")), subMeshData._name.c_str());
    }

    if (source->mNumBones > 0) {
        assert(source->mNumBones < std::numeric_limits<U8>::max());  ///<Fit in U8

        vectorImpl<vectorImpl<vertexWeight> > weightsPerVertex(source->mNumVertices);
        for (U8 a = 0; a < source->mNumBones; ++a) {
            const aiBone* bone = source->mBones[a];
            for (U32 b = 0; b < bone->mNumWeights; ++b) {
                weightsPerVertex[bone->mWeights[b].mVertexId].push_back(
                    vertexWeight(a, bone->mWeights[b].mWeight));
            }
        }

        vec4<F32> weights;
        P32       indices;
        for (U32 j = 0; j < source->mNumVertices; ++j) {
            U32 idx = j + previousOffset;

            indices.i = 0;
            weights.reset();
            // guaranteed to be max 4 thanks to aiProcess_LimitBoneWeights 
            for (U8 a = 0; a < weightsPerVertex[j].size(); ++a) {
                indices.b[a] = to_ubyte(weightsPerVertex[j][a]._boneID + submeshBoneOffsetOut);
                weights[a] = weightsPerVertex[j][a]._boneWeight;
            }

            targetBuffer->modifyBoneIndices(idx, indices);
            targetBuffer->modifyBoneWeights(idx, weights);
        }

        submeshBoneOffsetOut += to_ubyte(source->mNumBones);
    }

    U32 currentIndice = 0;
    vec3<U32> triangleTemp;

    subMeshData._triangles.reserve(source->mNumFaces * 3);
    for (U32 k = 0; k < source->mNumFaces; k++) {
        // guaranteed to be 3 thanks to aiProcess_Triangulate 
        for (U32 m = 0; m < 3; ++m) {
            currentIndice = source->mFaces[k].mIndices[m] + previousOffset;
            targetBuffer->addIndex(currentIndice);
            triangleTemp[m] = currentIndice;
        }

        subMeshData._triangles.push_back(triangleTemp);
    }

    subMeshData._partitionOffset = to_uint(targetBuffer->partitionBuffer());
}

void DVDConverter::loadSubMeshMaterial(Import::MaterialData& material,
                                       const aiMaterial* source,
                                       const stringImpl& materialName,
                                       const stringImpl& assetLocation,
                                       bool skinned) {

    material._name = materialName;
    Material::ColourData& data = material._colourData;
    // default diffuse colour
    data._diffuse.set(0.8f, 0.8f, 0.8f, 1.0f);
    // Load diffuse colour
    aiColor4D diffuse;
    if (AI_SUCCESS == aiGetMaterialColor(source, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
        data._diffuse.setV(&diffuse.r);
    } else {
        Console::d_printfn(Locale::get(_ID("MATERIAL_NO_DIFFUSE")), materialName.c_str());
    }

    // Ignore ambient colour
    vec4<F32> ambientTemp(0.0f, 0.0f, 0.0f, 1.0f);
    aiColor4D ambient;
    if (AI_SUCCESS == aiGetMaterialColor(source, AI_MATKEY_COLOR_AMBIENT, &ambient)) {
        ambientTemp.setV(&ambient.r);
    } else {
        // no ambient
    }

    // default specular colour
    data._specular.set(1.0f, 1.0f, 1.0f, 1.0f);
    // Load specular colour
    aiColor4D specular;
    if (AI_SUCCESS == aiGetMaterialColor(source, AI_MATKEY_COLOR_SPECULAR, &specular)) {
        data._specular.setV(&specular.r);
    } else {
        Console::d_printfn(Locale::get(_ID("MATERIAL_NO_SPECULAR")), materialName.c_str());
    }

    // default emissive colour
    data._emissive.set(0.0f, 0.0f, 0.0f, 1.0f);
    // Load emissive colour
    aiColor4D emissive;
    if (AI_SUCCESS == aiGetMaterialColor(source, AI_MATKEY_COLOR_EMISSIVE, &emissive)) {
        data._emissive.setV(&emissive.r);
    }

    // Load material opacity value
    aiGetMaterialFloat(source, AI_MATKEY_OPACITY, &data._diffuse.a);

    // default shading model
    I32 shadingModel = to_const_int(Material::ShadingMode::PHONG);
    // Load shading model
    aiGetMaterialInteger(source, AI_MATKEY_SHADING_MODEL, &shadingModel);
    material._shadingMode = aiShadingModeInternalTable[shadingModel];

    // Default shininess values
    F32 shininess = 15, strength = 1;
    // Load shininess
    aiGetMaterialFloat(source, AI_MATKEY_SHININESS, &shininess);
    // Load shininess strength
    aiGetMaterialFloat(source, AI_MATKEY_SHININESS_STRENGTH, &strength);
    F32 finalValue = shininess * strength;
    CLAMP<F32>(finalValue, 0.0f, 255.0f);
    data._shininess = finalValue;

    // check if material is two sided
    I32 two_sided = 0;
    aiGetMaterialInteger(source, AI_MATKEY_TWOSIDED, &two_sided);
    material._doubleSided = two_sided != 0;

    aiString tName;
    aiTextureMapping mapping;
    U32 uvInd;
    F32 blend;
    aiTextureOp op = aiTextureOp_Multiply;
    aiTextureMapMode mode[3] = {_aiTextureMapMode_Force32Bit,
                                _aiTextureMapMode_Force32Bit,
                                _aiTextureMapMode_Force32Bit};

    ParamHandler& par = ParamHandler::instance();

    U8 count = 0;
    // Compare load results with the standard success value
    aiReturn result = AI_SUCCESS;
    // While we still have diffuse textures

    stringImpl overridePath(par.getParam<stringImpl>(_ID("assetsLocation")) + "/" +
                            par.getParam<stringImpl>(_ID("defaultTextureLocation")) + "/");

    while (result == AI_SUCCESS) {
        // Load each one
        result = source->GetTexture(aiTextureType_DIFFUSE, count, &tName,
                                    &mapping, &uvInd, &blend, &op, mode);
        if (result != AI_SUCCESS) {
            break;
        }
        // get full path
        stringImpl path = overridePath + tName.data;
        // get only image name
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);
        // if we have a name and an extension
        if (!img_name.substr(img_name.find_first_of(".")).empty()) {

            ShaderProgram::TextureUsage usage = count == 1 ? ShaderProgram::TextureUsage::UNIT1
                                                           : ShaderProgram::TextureUsage::UNIT0;

            Import::TextureEntry& texture = material._textures[to_uint(usage)];

            // Load the texture resource
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                texture._wrapU = aiTextureMapModeTable[mode[0]];
                texture._wrapV = aiTextureMapModeTable[mode[1]];
                texture._wrapW = aiTextureMapModeTable[mode[2]];
            }
            texture._textureName = img_name;
            texture._texturePath = path;
            // The first texture is always "Replace"
            texture._operation = count == 0 ? Material::TextureOperation::REPLACE
                                            : aiTextureOperationTable[op];
            material._textures[to_uint(usage)] = texture;
                                                                
        }  // endif

        tName.Clear();
        count++;
        if (count == 2) {
            break;
        }
        STUBBED("ToDo: Use more than 2 textures for each material. Fix This! -Ionut")
    }  // endwhile

    result = source->GetTexture(aiTextureType_NORMALS, 0, &tName, &mapping,
                                &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        stringImpl path = overridePath + tName.data;
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);

        Import::TextureEntry& texture = material._textures[to_const_uint(ShaderProgram::TextureUsage::NORMALMAP)];

        if (img_name.rfind('.') != stringImpl::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                texture._wrapU = aiTextureMapModeTable[mode[0]];
                texture._wrapV = aiTextureMapModeTable[mode[1]];
                texture._wrapW = aiTextureMapModeTable[mode[2]];
            }
            texture._textureName = img_name;
            texture._texturePath = path;
            texture._operation = aiTextureOperationTable[op];
            material._textures[to_const_uint(ShaderProgram::TextureUsage::NORMALMAP)] = texture;
            material._bumpMethod = Material::BumpMethod::NORMAL;
        }  // endif
    } // endif

    result = source->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping,
                                &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        stringImpl path = overridePath + tName.data;
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);
        Import::TextureEntry& texture = material._textures[to_const_uint(ShaderProgram::TextureUsage::NORMALMAP)];
        if (img_name.rfind('.') != stringImpl::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                texture._wrapU = aiTextureMapModeTable[mode[0]];
                texture._wrapV = aiTextureMapModeTable[mode[1]];
                texture._wrapW = aiTextureMapModeTable[mode[2]];
            }
            texture._textureName = img_name;
            texture._texturePath = path;
            texture._operation = aiTextureOperationTable[op];
            material._textures[to_const_uint(ShaderProgram::TextureUsage::NORMALMAP)] = texture;
            material._bumpMethod = Material::BumpMethod::NORMAL;
        }  // endif
    } // endif

    I32 flags = 0;
    aiGetMaterialInteger(source, AI_MATKEY_TEXFLAGS_DIFFUSE(0), &flags);
    material._ignoreAlpha = (flags & aiTextureFlags_IgnoreAlpha) != 0;

    if (!material._ignoreAlpha) {
        result = source->GetTexture(aiTextureType_OPACITY, 0, &tName, &mapping,
                                    &uvInd, &blend, &op, mode);
        if (result == AI_SUCCESS) {
            stringImpl path = overridePath + tName.data;
            stringImpl img_name = path.substr(path.find_last_of('/') + 1);
            Import::TextureEntry& texture = material._textures[to_const_uint(ShaderProgram::TextureUsage::OPACITY)];

            if (img_name.rfind('.') != stringImpl::npos) {
                if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                          aiTextureMapMode_Decal) &&
                    IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                          aiTextureMapMode_Decal) &&
                    IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                          aiTextureMapMode_Decal)) {
                    texture._wrapU = aiTextureMapModeTable[mode[0]];
                    texture._wrapV = aiTextureMapModeTable[mode[1]];
                    texture._wrapW = aiTextureMapModeTable[mode[2]];
                }
                texture._textureName = img_name;
                texture._texturePath = path;
                texture._operation = aiTextureOperationTable[op];
                material._textures[to_const_uint(ShaderProgram::TextureUsage::OPACITY)] = texture;
                material._doubleSided = true;
            }  // endif
        } 
    }

    result = source->GetTexture(aiTextureType_SPECULAR, 0, &tName, &mapping,
                                &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        stringImpl path = overridePath + tName.data;
        stringImpl img_name = path.substr(path.find_last_of('/') + 1);
        Import::TextureEntry& texture = material._textures[to_const_uint(ShaderProgram::TextureUsage::SPECULAR)];
        if (img_name.rfind('.') != stringImpl::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap,
                                      aiTextureMapMode_Decal)) {
                texture._wrapU = aiTextureMapModeTable[mode[0]];
                texture._wrapV = aiTextureMapModeTable[mode[1]];
                texture._wrapW = aiTextureMapModeTable[mode[2]];
            }
            texture._textureName = img_name;
            texture._texturePath = path;
            texture._operation = aiTextureOperationTable[op];
            material._textures[to_const_uint(ShaderProgram::TextureUsage::SPECULAR)] = texture;
        }  // endif
    }      // endif
}
};