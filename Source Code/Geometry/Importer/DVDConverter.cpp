#include "stdafx.h"

#include "config.h"

#include "Headers/DVDConverter.h"
#include "Headers/MeshImporter.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/StringHelper.h"
#include "Geometry/Animations/Headers/AnimationUtils.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Utility/Headers/Localization.h"

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/Logger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/pbrmaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>

#include <meshoptimizer/src/meshoptimizer.h>

namespace Divide {

namespace {
        constexpr bool g_useSloppyMeshSimplification = false;
        constexpr U8 g_SloppyTrianglePercentPerLoD = 80;
        constexpr U8 g_PreciseTrianglePercentPerLoD = 70;
        constexpr size_t g_minIndexCountForAutoLoD = 1024;

        std::atomic_bool g_wasSetUp = false;

        struct assimpStream final : public Assimp::LogStream {
            void write(const char* message) final {
                Console::printf("%s\n", message);
            }
        };

        // Select the kinds of messages you want to receive on this log stream
        constexpr U32 severity = Config::Build::IS_DEBUG_BUILD ? Assimp::Logger::VERBOSE : Assimp::Logger::NORMAL;

        constexpr bool g_removeLinesAndPoints = true;

        struct vertexWeight {
            U8 _boneID = 0;
            F32 _boneWeight = 0.0f;
        };

        /// Recursively creates an internal node structure matching the current scene and animation.
        Bone* createBoneTree(aiNode* pNode, Bone* parent) {
            Bone* internalNode = MemoryManager_NEW Bone(pNode->mName.data);
            // set the parent; in case this is the root node, it will be null
            internalNode->_parent = parent;

            AnimUtils::TransformMatrix(pNode->mTransformation, internalNode->_localTransform);
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

    hashMap<U32, TextureWrap> DVDConverter::fillTextureWrapMap() {
        hashMap<U32, TextureWrap> wrapMap;
        wrapMap[aiTextureMapMode_Wrap] = TextureWrap::CLAMP;
        wrapMap[aiTextureMapMode_Clamp] = TextureWrap::CLAMP_TO_EDGE;
        wrapMap[aiTextureMapMode_Mirror] = TextureWrap::REPEAT;
        wrapMap[aiTextureMapMode_Decal] = TextureWrap::DECAL;
        return wrapMap;
    }

    hashMap<U32, ShadingMode> DVDConverter::fillShadingModeMap() {
        hashMap<U32, ShadingMode> shadingMap;
        shadingMap[aiShadingMode_Fresnel] = ShadingMode::COOK_TORRANCE;
        shadingMap[aiShadingMode_NoShading] = ShadingMode::FLAT;
        shadingMap[aiShadingMode_CookTorrance] = ShadingMode::COOK_TORRANCE;
        shadingMap[aiShadingMode_Minnaert] = ShadingMode::OREN_NAYAR;
        shadingMap[aiShadingMode_OrenNayar] = ShadingMode::OREN_NAYAR;
        shadingMap[aiShadingMode_Toon] = ShadingMode::TOON;
        shadingMap[aiShadingMode_Blinn] = ShadingMode::BLINN_PHONG;
        shadingMap[aiShadingMode_Phong] = ShadingMode::PHONG;
        shadingMap[aiShadingMode_Gouraud] = ShadingMode::BLINN_PHONG;
        shadingMap[aiShadingMode_Flat] = ShadingMode::FLAT;
        return shadingMap;
    }

    hashMap<U32, TextureOperation> DVDConverter::fillTextureOperationMap() {
        hashMap<U32, TextureOperation> operationMap;
        operationMap[aiTextureOp_Multiply] = TextureOperation::MULTIPLY;
        operationMap[aiTextureOp_Add] = TextureOperation::ADD;
        operationMap[aiTextureOp_Subtract] = TextureOperation::SUBTRACT;
        operationMap[aiTextureOp_Divide] = TextureOperation::DIVIDE;
        operationMap[aiTextureOp_SmoothAdd] = TextureOperation::SMOOTH_ADD;
        operationMap[aiTextureOp_SignedAdd] = TextureOperation::SIGNED_ADD;
        operationMap[/*aiTextureOp_Replace*/ 7] = TextureOperation::REPLACE;
        return operationMap;
    }


    hashMap<U32, TextureWrap>
    DVDConverter::aiTextureMapModeTable = DVDConverter::fillTextureWrapMap();
    hashMap<U32, ShadingMode>
    DVDConverter::aiShadingModeInternalTable = DVDConverter::fillShadingModeMap();
    hashMap<U32, TextureOperation>
    DVDConverter::aiTextureOperationTable = DVDConverter::fillTextureOperationMap();

DVDConverter::DVDConverter(PlatformContext& context, Import::ImportData& target, bool& result) {
    bool expected = false;
    if (g_wasSetUp.compare_exchange_strong(expected, true)) {
        Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
        Assimp::DefaultLogger::get()->attachStream(new assimpStream(), severity);
    }

    result = load(context, target);
}

bool DVDConverter::load(PlatformContext& context, Import::ImportData& target) {
    const ResourcePath& filePath = target.modelPath();
    const ResourcePath& fileName = target.modelName();

    Assimp::Importer importer;

    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, g_removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT : 0);
    //importer.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_SEARCH_EMBEDDED_TEXTURES, 1);
    importer.SetPropertyInteger(AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);

    constexpr U32 ppsteps = aiProcess_CalcTangentSpace |
                            aiProcess_JoinIdenticalVertices |
                            aiProcess_ImproveCacheLocality |
                            aiProcess_GenSmoothNormals |
                            aiProcess_LimitBoneWeights |
                            aiProcess_RemoveRedundantMaterials |
                             //aiProcess_FixInfacingNormals | // Causes issues with backfaces inside the Sponza Atrium model
                            aiProcess_SplitLargeMeshes |
                            aiProcess_FindInstances |
                            aiProcess_Triangulate |
                            aiProcess_GenUVCoords |
                            aiProcess_SortByPType |
                            aiProcess_FindDegenerates |
                            aiProcess_FindInvalidData |
                            aiProcess_ValidateDataStructure |
                            aiProcess_OptimizeMeshes |
                            aiProcess_TransformUVCoords;// Preprocess UV transformations (scaling, translation ...)

    const aiScene* aiScenePointer = importer.ReadFile(((filePath.str() + "/") + fileName.str()).c_str(), ppsteps);

    if (!aiScenePointer) {
        Console::errorfn(Locale::get(_ID("ERROR_IMPORTER_FILE")), fileName.c_str(), importer.GetErrorString());
        return false;
    }

    target.hasAnimations(aiScenePointer->HasAnimations());
    if (target.hasAnimations()) {
        target.skeleton(createBoneTree(aiScenePointer->mRootNode, nullptr));
        target._bones.reserve(to_I32(target.skeleton()->hierarchyDepth()));

        for (U16 meshPointer = 0; meshPointer < aiScenePointer->mNumMeshes; ++meshPointer) {
            const aiMesh* mesh = aiScenePointer->mMeshes[meshPointer];
            for (U32 n = 0; n < mesh->mNumBones; ++n) {
                const aiBone* bone = mesh->mBones[n];

                Bone* found = target.skeleton()->find(bone->mName.data);
                if (found != nullptr) {
                    AnimUtils::TransformMatrix(bone->mOffsetMatrix, found->_offsetMatrix);
                    target._bones.push_back(found);
                }
            }
        }

        for (size_t i(0); i < aiScenePointer->mNumAnimations; i++) {
            aiAnimation* animation = aiScenePointer->mAnimations[i];
            if (animation->mDuration > 0.0f) {
                target._animations.push_back(std::make_shared<AnimEvaluator>(animation, to_U32(i)));
            }
        }
    }

    const U32 numMeshes = aiScenePointer->mNumMeshes;
    target._subMeshData.reserve(numMeshes);

    Str64 prevName = "";
    for (U16 n = 0; n < numMeshes; ++n) {
        aiMesh* currentMesh = aiScenePointer->mMeshes[n];
        // Skip points and lines ... for now -Ionut
        if (currentMesh->mNumVertices == 0) {
            continue;
        }

        stringImpl fullName = currentMesh->mName.C_Str();
        if (fullName.length() >= 64) {
            fullName = fullName.substr(0, 64);
        }

        Str64 name = fullName.c_str();
        if (Util::CompareIgnoreCase(name, "defaultobject")) {
            name.append("_" + fileName.str());
        }

        Import::SubMeshData subMeshTemp = {};
        subMeshTemp.name(name);
        subMeshTemp.index(to_U32(n));
        if (subMeshTemp.name().empty()) {
            subMeshTemp.name(Util::StringFormat("%s-submesh-%d", fileName.c_str(), n).c_str());
        }
        if (subMeshTemp.name() == prevName) {
            subMeshTemp.name(prevName + "_" + Util::to_string(n).c_str());
        }

        prevName = subMeshTemp.name();
        subMeshTemp.boneCount(currentMesh->mNumBones);
        loadSubMeshGeometry(currentMesh, subMeshTemp);

        loadSubMeshMaterial(subMeshTemp._material,
                            aiScenePointer,
                            to_U16(currentMesh->mMaterialIndex),
                            Str128(subMeshTemp.name()) + "_material",
                            true);


        target._subMeshData.push_back(subMeshTemp);
    }

    buildGeometryBuffers(context, target);
    return true;
}

void DVDConverter::buildGeometryBuffers(PlatformContext& context, Import::ImportData& target) {
    target.vertexBuffer(context.gfx().newVB());
    VertexBuffer* vb = target.vertexBuffer();

    size_t indexCount = 0, vertexCount = 0;
    for (U8 lod = 0; lod < Import::MAX_LOD_LEVELS; ++lod) {
        for (const Import::SubMeshData& data : target._subMeshData) {
            indexCount += data._indices[lod].size();
            vertexCount += data._vertices[lod].size();
        }
    }

    vb->useLargeIndices(vertexCount + 1 > std::numeric_limits<U16>::max());
    vb->setVertexCount(vertexCount);
    vb->reserveIndexCount(indexCount);

    U32 previousOffset = 0;
    for (U8 lod = 0; lod < Import::MAX_LOD_LEVELS; ++lod) {
        U8 submeshBoneOffset = 0;
        for (Import::SubMeshData& data : target._subMeshData) {
            const size_t idxCount = data._indices[lod].size();

            if (idxCount == 0) {
                assert(lod > 0);
                submeshBoneOffset += to_U8(data.boneCount());
                data._partitionIDs[lod] = data._partitionIDs[lod - 1];
                continue;
            }

            data._triangles[lod].reserve(idxCount / 3);
            const auto& indices = data._indices[lod];
            for (size_t i = 0; i < idxCount; i += 3) {
                U32 triangleTemp[3] = {
                    indices[i + 0] + previousOffset,
                    indices[i + 1] + previousOffset,
                    indices[i + 2] + previousOffset
                };

                data._triangles[lod].emplace_back(triangleTemp[0], triangleTemp[1], triangleTemp[2]);

                vb->addIndex(triangleTemp[0]);
                vb->addIndex(triangleTemp[1]);
                vb->addIndex(triangleTemp[2]);
            }

            auto& vertices = data._vertices[lod];
            const U32 vertCount = to_U32(vertices.size());

            const bool hasBones = data.boneCount() > 0;
            const bool hasTexCoord = !IS_ZERO(vertices[0].texcoord.z);
            const bool hasTangent = !IS_ZERO(vertices[0].tangent.w);

            BoundingBox importBB = {};
            for (U32 i = 0; i < vertCount; ++i) {
                const U32 targetIdx = i + previousOffset;

                const Import::SubMeshData::Vertex& vert = vertices[i];

                vb->modifyPositionValue(targetIdx, vert.position);
                vb->modifyNormalValue(targetIdx, vert.normal);

                if (hasTexCoord) {
                    vb->modifyTexCoordValue(targetIdx, vert.texcoord.xy);
                }
                if (hasTangent) {
                    vb->modifyTangentValue(targetIdx, vert.tangent.xyz);
                }
                if (lod == 0) {
                    importBB.add(vert.position);
                }
            }//vertCount

            if (hasBones) {
                for (U32 i = 0; i < vertCount; ++i) {
                    const U32 targetIdx = i + previousOffset;

                    Import::SubMeshData::Vertex& vert = vertices[i];
                    P32& boneIndices = vert.indices;
                    for (U8& idx : boneIndices.b) {
                        idx += submeshBoneOffset;
                    }

                    vb->modifyBoneIndices(targetIdx, boneIndices);
                    vb->modifyBoneWeights(targetIdx, vert.weights);
                }
            }

            if (lod == 0) {
                data.minPos(importBB.getMin());
                data.maxPos(importBB.getMax());
            }

            submeshBoneOffset += to_U8(data.boneCount());
            previousOffset += to_U32(vertices.size());
            data._partitionIDs[lod] = vb->partitionBuffer();
        } //submesh data
    } //lod
}

void DVDConverter::loadSubMeshGeometry(const aiMesh* source, Import::SubMeshData& subMeshData) {
    vectorEASTL<U32> input_indices;
    input_indices.reserve(source->mNumFaces * 3);
    for (U32 k = 0; k < source->mNumFaces; k++) {
        // guaranteed to be 3 thanks to aiProcess_Triangulate 
        for (U32 m = 0; m < 3; ++m) {
            input_indices.push_back(source->mFaces[k].mIndices[m]);
        }
    }

    vectorEASTL<Import::SubMeshData::Vertex> vertices(source->mNumVertices);

    for (U32 j = 0; j < source->mNumVertices; ++j) {
        vertices[j].position.set(source->mVertices[j].x, source->mVertices[j].y, source->mVertices[j].z);
        vertices[j].normal.set(source->mNormals[j].x, source->mNormals[j].y, source->mNormals[j].z);
    }

    if (source->mTextureCoords[0] != nullptr) {
        for (U32 j = 0; j < source->mNumVertices; ++j) {
            vertices[j].texcoord.set(source->mTextureCoords[0][j].x, source->mTextureCoords[0][j].y, 1.0f);
        }
    }

    if (source->mTangents != nullptr) {
        for (U32 j = 0; j < source->mNumVertices; ++j) {
            vertices[j].tangent.set(source->mTangents[j].x, source->mTangents[j].y, source->mTangents[j].z, 1.0f);
        }
    } else {
        Console::d_printfn(Locale::get(_ID("SUBMESH_NO_TANGENT")), subMeshData.name().c_str());
    }

    if (source->mNumBones > 0) {
        assert(source->mNumBones < std::numeric_limits<U8>::max());  ///<Fit in U8

        vectorEASTL<vectorEASTL<vertexWeight> > weightsPerVertex(source->mNumVertices);
        for (U8 a = 0; a < source->mNumBones; ++a) {
            const aiBone* bone = source->mBones[a];
            for (U32 b = 0; b < bone->mNumWeights; ++b) {
                weightsPerVertex[bone->mWeights[b].mVertexId].push_back({ a, bone->mWeights[b].mWeight });
            }
        }

        vec4<F32> weights;
        P32       indices;
        for (U32 j = 0; j < source->mNumVertices; ++j) {
            indices.i = 0;
            weights.reset();
            // guaranteed to be max 4 thanks to aiProcess_LimitBoneWeights 
            for (size_t a = 0; a < weightsPerVertex[j].size(); ++a) {
                indices.b[a] = to_U8(weightsPerVertex[j][a]._boneID);
                weights[a] = weightsPerVertex[j][a]._boneWeight;
            }
            vertices[j].indices = indices;
            vertices[j].weights.set(weights);
        }
    }

    auto& target_indices = subMeshData._indices[0];
    auto& target_vertices = subMeshData._vertices[0];

    constexpr F32 kThreshold = 1.01f; // allow up to 1% worse ACMR to get more reordering opportunities for overdraw

    const size_t index_count = input_indices.size();
    target_indices.resize(index_count);

    { //Remap VB & IB
        vectorEASTL<U32> remap(source->mNumVertices);
        const size_t vertex_count = meshopt_generateVertexRemap(&remap[0], input_indices.data(), input_indices.size(), &vertices[0], source->mNumVertices, sizeof(Import::SubMeshData::Vertex));

        meshopt_remapIndexBuffer(&target_indices[0], &input_indices[0], input_indices.size(), &remap[0]);
        input_indices.clear();

        target_vertices.resize(vertex_count);
        meshopt_remapVertexBuffer(&target_vertices[0], &vertices[0], source->mNumVertices, sizeof(Import::SubMeshData::Vertex), &remap[0]);
        vertices.clear();
        remap.clear();
    }
    { // Optimise VB & IB
        meshopt_optimizeVertexCache(&target_indices[0], &target_indices[0], index_count, target_vertices.size());


        meshopt_optimizeOverdraw(&target_indices[0], &target_indices[0], index_count, &target_vertices[0].position.x, target_vertices.size(), sizeof(Import::SubMeshData::Vertex), kThreshold);

        // vertex fetch optimization should go last as it depends on the final index order
        meshopt_optimizeVertexFetch(&target_vertices[0], &target_indices[0], index_count, &target_vertices[0], target_vertices.size(), sizeof(Import::SubMeshData::Vertex));
    }
    { // Generate LoD data and place inside VB & IB with proper offsets
        // We only simplify from the base level. Simplifying from the previous level might be faster,
        // but we cache the geometry anyway at the end
        auto& source_indices = subMeshData._indices[0];
        auto& source_vertices = subMeshData._vertices[0];

        const F32 targetSimplification = (g_useSloppyMeshSimplification ? g_SloppyTrianglePercentPerLoD : g_PreciseTrianglePercentPerLoD) * 0.01f;

        if (source_indices.size() >= g_minIndexCountForAutoLoD) {
            for (U8 i = 1; i < Import::MAX_LOD_LEVELS; ++i) {
                auto& lod_indices = subMeshData._indices[i];

                const F32 threshold = std::pow(targetSimplification, to_F32(i));

                const size_t target_index_count = std::min(source_indices.size(), to_size(index_count * threshold) / 3 * 3);

                lod_indices.resize(source_indices.size());
                if (g_useSloppyMeshSimplification) {
                    lod_indices.resize(meshopt_simplifySloppy(lod_indices.data(),
                                                              source_indices.data(),
                                                              source_indices.size(),
                                                              &source_vertices[0].position.x,
                                                              source_vertices.size(),
                                                              sizeof(Import::SubMeshData::Vertex),
                                                              target_index_count));
                } else {
                    constexpr F32 target_error = 1e-2f;
                    lod_indices.resize(meshopt_simplify(lod_indices.data(),
                                                        source_indices.data(),
                                                        source_indices.size(),
                                                        &source_vertices[0].position.x,
                                                        source_vertices.size(),
                                                        sizeof(Import::SubMeshData::Vertex),
                                                        target_index_count,
                                                        target_error));
                }
            }

            // reorder indices for overdraw, balancing overdraw and vertex cache efficiency
            for (U8 i = 1; i < Import::MAX_LOD_LEVELS; ++i) {
                auto& lod_indices = subMeshData._indices[i];
                auto& lod_vertices = subMeshData._vertices[i];

                meshopt_optimizeVertexCache(lod_indices.data(),
                                            lod_indices.data(),
                                            lod_indices.size(),
                                            source_vertices.size());

                meshopt_optimizeOverdraw(lod_indices.data(),
                                         lod_indices.data(),
                                         lod_indices.size(),
                                         &source_vertices[0].position.x,
                                         source_vertices.size(),
                                         sizeof(Import::SubMeshData::Vertex),
                                         1.0f);

                lod_vertices.resize(source_vertices.size());
                lod_vertices.resize(meshopt_optimizeVertexFetch(lod_vertices.data(),
                                                                lod_indices.data(),
                                                                lod_indices.size(),
                                                                source_vertices.data(),
                                                                source_vertices.size(),
                                                                sizeof(Import::SubMeshData::Vertex)));
            }
        }
    }
}

void DVDConverter::loadSubMeshMaterial(Import::MaterialData& material,
                                       const aiScene* source,
                                       const U16 materialIndex,
                                       const Str128& materialName,
                                       bool convertHeightToBumpMap) const
{

    const aiMaterial* mat = source->mMaterials[materialIndex];

    material.name(materialName);

    // default shading model
    I32 shadingModel = to_base(ShadingMode::PHONG);
    // Load shading model
    aiGetMaterialInteger(mat, AI_MATKEY_SHADING_MODEL, &shadingModel);
    material.shadingMode(aiShadingModeInternalTable[shadingModel]);

    const bool isPBRMaterial = !(material.shadingMode() != ShadingMode::OREN_NAYAR && 
                                 material.shadingMode() != ShadingMode::COOK_TORRANCE);
    
    // Load material opacity value
    F32 alpha = 1.0f;
    aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &alpha);

    // default diffuse colour
    material.baseColour(FColour4(0.8f, 0.8f, 0.8f, 1.0f));
    // Load diffuse colour
    aiColor4D diffuse;
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
        material.baseColour(FColour4(diffuse.r, diffuse.g, diffuse.b, alpha));
    } else {
        Console::d_printfn(Locale::get(_ID("MATERIAL_NO_DIFFUSE")), materialName.c_str());
    }

    // default emissive colour
    material.emissive(FColour3(0.0f, 0.0f, 0.0f));
    // Load emissive colour
    aiColor4D emissive;
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &emissive)) {
        material.emissive(FColour3(emissive.r, emissive.g, emissive.b));
    }

    // Ignore ambient colour
    vec4<F32> ambientTemp(0.0f, 0.0f, 0.0f, 1.0f);
    aiColor4D ambient;
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &ambient)) {
        ambientTemp.set(&ambient.r);
    } else {
        // no ambient
    }

    // Default shininess values
    F32 roughness = 0.0f, metallic = 0.0f;
    // Load shininess
    if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &metallic)) {
        material.metallic(metallic);
    } else {
    }

    F32 bumpScalling = 0.0f;
    if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_BUMPSCALING, &bumpScalling)) {
        material.parallaxFactor(bumpScalling);
    }
    
    if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &roughness)) {
        material.roughness(roughness);
    } else {
        // Load specular colour as a hack :/
        aiColor4D specular;
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &specular)) {
            material.roughness(1.0f - CLAMPED_01(FColour3(specular.r, specular.g, specular.b).maxComponent()));
        } else {
            F32 shininess = 0.f, strength = 1.f;
            if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_SHININESS_STRENGTH, &strength) &&
                AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &shininess)) {
                F32 temp = CLAMPED(shininess * strength, 0.f, 128.f);
                if (temp > 1.f) {
                    temp /= 128.0f;
                }

                material.roughness(1.0f - CLAMPED_01(temp));
            }
        }
    }

       
    // check if material is two sided
    I32 two_sided = 0;
    aiGetMaterialInteger(mat, AI_MATKEY_TWOSIDED, &two_sided);
    material.doubleSided(two_sided != 0);

    aiString tName;
    aiTextureMapping mapping = aiTextureMapping_OTHER;
    U32 uvInd = 0;
    F32 blend = 0.0f;
    aiTextureOp op = aiTextureOp_Multiply;
    aiTextureMapMode mode[3] = {_aiTextureMapMode_Force32Bit,
                                _aiTextureMapMode_Force32Bit,
                                _aiTextureMapMode_Force32Bit};

    U8 count = 0;
    // Compare load results with the standard success value
    aiReturn result = AI_SUCCESS;
    // While we still have diffuse textures
    while (result == AI_SUCCESS) {
        // Load each one
        result = mat->GetTexture(aiTextureType_DIFFUSE, count, &tName, &mapping, &uvInd, &blend, &op, mode);

        if (result != AI_SUCCESS) {
            break;
        }
        if (tName.length == 0) {
            break;
        }

        // it might be an embedded texture
        /*const aiTexture* texture = mat->GetEmbeddedTexture(tName.C_Str());
        
        if (texture != nullptr )
        {
        }*/

        // get full path
        const ResourcePath path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.C_Str());

        auto [img_name, img_path] = splitPathToNameAndLocation(path);

        // if we have a name and an extension
        if (!img_name.str().substr(img_name.str().find_first_of("."), Str64::npos).empty()) {

            const TextureUsage usage = count == 1 ? TextureUsage::UNIT1
                                                                 : TextureUsage::UNIT0;

            Import::TextureEntry& texture = material._textures[to_U32(usage)];

            // Load the texture resource
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap, aiTextureMapMode_Decal))
            {
                texture.wrapU(aiTextureMapModeTable[mode[0]]);
                texture.wrapV(aiTextureMapModeTable[mode[1]]);
                texture.wrapW(aiTextureMapModeTable[mode[2]]);
            }

            texture.textureName(img_name);
            texture.texturePath(img_path);
            // The first texture is always "Replace"
            texture.operation(count == 0 ? TextureOperation::NONE
                                         : aiTextureOperationTable[op]);
            texture.srgb(true);
            material._textures[to_U32(usage)] = texture;
                                                                
        }  // endif

        tName.Clear();
        count++;
        if (count == 2) {
            break;
        }

        STUBBED("ToDo: Use more than 2 textures for each material. Fix This! -Ionut")
    }  // endwhile

    bool hasNormalMap = false;
    result = mat->GetTexture(aiTextureType_NORMALS, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        const ResourcePath path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.C_Str());

        auto [img_name, img_path] = splitPathToNameAndLocation(path);

        Import::TextureEntry& texture = material._textures[to_base(TextureUsage::NORMALMAP)];

        if (img_name.str().rfind('.') != Str64::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap, aiTextureMapMode_Decal))
            {
                texture.wrapU(aiTextureMapModeTable[mode[0]]);
                texture.wrapV(aiTextureMapModeTable[mode[1]]);
                texture.wrapW(aiTextureMapModeTable[mode[2]]);
            }
            texture.textureName(img_name);
            texture.texturePath(img_path);
            texture.operation(aiTextureOperationTable[op]);
            texture.srgb(false);

            material._textures[to_base(TextureUsage::NORMALMAP)] = texture;
            material.bumpMethod(BumpMethod::NORMAL);
            hasNormalMap = true;
        }  // endif
    } // endif

    result = mat->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping,&uvInd, &blend, &op, mode);

    if (result == AI_SUCCESS) {
        const ResourcePath path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.C_Str());

        auto [img_name, img_path] = splitPathToNameAndLocation(path);

        Import::TextureEntry& texture = material._textures[to_base(TextureUsage::HEIGHTMAP)];
        if (img_name.str().rfind('.') != Str64::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap, aiTextureMapMode_Decal))
            {
                texture.wrapU(aiTextureMapModeTable[mode[0]]);
                texture.wrapV(aiTextureMapModeTable[mode[1]]);
                texture.wrapW(aiTextureMapModeTable[mode[2]]);
            }
            texture.textureName(img_name);
            texture.texturePath(img_path);
            texture.operation(aiTextureOperationTable[op]);
            texture.srgb(false);
            if (convertHeightToBumpMap && !hasNormalMap) {
                material._textures[to_base(TextureUsage::NORMALMAP)] = texture;
                material.bumpMethod(BumpMethod::NORMAL);
                hasNormalMap = true;
            } else {
                material._textures[to_base(TextureUsage::HEIGHTMAP)] = texture;
                material.bumpMethod(BumpMethod::PARALLAX);
            }
        }  // endif
    } // endif

    I32 flags = 0;
    aiGetMaterialInteger(mat, AI_MATKEY_TEXFLAGS_DIFFUSE(0), &flags);
    material.ignoreAlpha((flags & aiTextureFlags_IgnoreAlpha) != 0);

    if (!material.ignoreAlpha()) {
        result = mat->GetTexture(aiTextureType_OPACITY, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
        if (result == AI_SUCCESS) {
            const ResourcePath path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.C_Str());

            auto [img_name, img_path] = splitPathToNameAndLocation(path);

            Import::TextureEntry& texture = material._textures[to_base(TextureUsage::OPACITY)];

            if (img_name.str().rfind('.') != Str64::npos) {
                if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                    IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                    IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap, aiTextureMapMode_Decal))
                {
                    texture.wrapU(aiTextureMapModeTable[mode[0]]);
                    texture.wrapV(aiTextureMapModeTable[mode[1]]);
                    texture.wrapW(aiTextureMapModeTable[mode[2]]);
                }
                texture.textureName(img_name);
                texture.texturePath(img_path);
                texture.operation(aiTextureOperationTable[op]);
                texture.srgb(false);
                material.doubleSided(true);
                material._textures[to_base(TextureUsage::OPACITY)] = texture;
            }  // endif
        } 
    }

    constexpr aiTextureType specularSource[] = {
        aiTextureType_METALNESS,
        aiTextureType_DIFFUSE_ROUGHNESS,
        aiTextureType_AMBIENT_OCCLUSION,
        aiTextureType_SPECULAR
    };

    result = mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &tName, &mapping, &uvInd, &blend, &op, mode);
    if (result != AI_SUCCESS) {
        for (aiTextureType t : specularSource) {
            result = mat->GetTexture(t, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
            if (result == AI_SUCCESS) {
                break;
            }
        }
    }
    if (result == AI_SUCCESS) {
        const ResourcePath path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.C_Str());

        auto [img_name, img_path] = splitPathToNameAndLocation(path);

        Import::TextureEntry& texture = material._textures[to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS)];
        if (img_name.str().rfind('.') != Str64::npos) {
            if (IS_IN_RANGE_INCLUSIVE(mode[0], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[1], aiTextureMapMode_Wrap, aiTextureMapMode_Decal) &&
                IS_IN_RANGE_INCLUSIVE(mode[2], aiTextureMapMode_Wrap, aiTextureMapMode_Decal)) {
                texture.wrapU(aiTextureMapModeTable[mode[0]]);
                texture.wrapV(aiTextureMapModeTable[mode[1]]);
                texture.wrapW(aiTextureMapModeTable[mode[2]]);
            }
            texture.textureName(img_name);
            texture.texturePath(img_path);
            texture.operation(aiTextureOperationTable[op]);
            texture.srgb(false);

            material._textures[to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS)] = texture;
        }
    }
}

};