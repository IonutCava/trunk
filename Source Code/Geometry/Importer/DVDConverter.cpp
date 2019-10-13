#include "stdafx.h"

#include "config.h"

#include "Headers/DVDConverter.h"
#include "Headers/MeshImporter.h"

#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "Geometry/Animations/Headers/AnimationUtils.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"


#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/Logger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/pbrmaterial.h>

#include <meshoptimizer/src/meshoptimizer.h>

namespace Divide {
    namespace {
        bool g_wasSetUp = false;

        class assimpStream : public Assimp::LogStream {
        public:
            assimpStream() = default;
            ~assimpStream() = default;

            void write(const char* message) {
                Console::printf("%s\n", message);
            }
        };

        // Select the kinds of messages you want to receive on this log stream
        const U32 severity = Config::Build::IS_DEBUG_BUILD ? Assimp::Logger::VERBOSE : Assimp::Logger::NORMAL;

        constexpr bool g_removeLinesAndPoints = true;

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

    hashMap<U32, Material::ShadingMode> DVDConverter::fillShadingModeMap() {
        hashMap<U32, Material::ShadingMode> shadingMap;
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

    hashMap<U32, Material::TextureOperation> DVDConverter::fillTextureOperationMap() {
        hashMap<U32, Material::TextureOperation> operationMap;
        operationMap[aiTextureOp_Multiply] = Material::TextureOperation::MULTIPLY;
        operationMap[aiTextureOp_Add] = Material::TextureOperation::ADD;
        operationMap[aiTextureOp_Subtract] = Material::TextureOperation::SUBTRACT;
        operationMap[aiTextureOp_Divide] = Material::TextureOperation::DIVIDE;
        operationMap[aiTextureOp_SmoothAdd] = Material::TextureOperation::SMOOTH_ADD;
        operationMap[aiTextureOp_SignedAdd] = Material::TextureOperation::SIGNED_ADD;
        operationMap[/*aiTextureOp_Replace*/ 7] = Material::TextureOperation::REPLACE;
        return operationMap;
    }


    hashMap<U32, TextureWrap>
    DVDConverter::aiTextureMapModeTable = DVDConverter::fillTextureWrapMap();
    hashMap<U32, Material::ShadingMode>
    DVDConverter::aiShadingModeInternalTable = DVDConverter::fillShadingModeMap();
    hashMap<U32, Material::TextureOperation>
    DVDConverter::aiTextureOperationTable = DVDConverter::fillTextureOperationMap();

DVDConverter::DVDConverter()
{
}

DVDConverter::DVDConverter(PlatformContext& context, Import::ImportData& target, bool& result) {
    if (!g_wasSetUp) {
        g_wasSetUp = true;
        Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
        Assimp::DefaultLogger::get()->attachStream(new assimpStream(), severity);
    }

    result = load(context, target);
}

DVDConverter::~DVDConverter()
{
}

bool DVDConverter::load(PlatformContext& context, Import::ImportData& target) {
    const stringImpl& filePath = target.modelPath();
    const stringImpl& fileName = target.modelName();

    Assimp::Importer importer;

    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
                                g_removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT
                                                       : 0);

    //importer.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_SEARCH_EMBEDDED_TEXTURES, 1);
    importer.SetPropertyInteger(AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);

    U32 ppsteps = aiProcessPreset_TargetRealtime_MaxQuality |
                  aiProcess_TransformUVCoords;// Preprocess UV transformations (scaling, translation ...)

    const aiScene* aiScenePointer = importer.ReadFile((filePath + "/" + fileName).c_str(), ppsteps);

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

    U32 numMeshes = aiScenePointer->mNumMeshes;
    target._subMeshData.reserve(numMeshes);

    stringImpl prevName = "";
    for (U16 n = 0; n < numMeshes; ++n) {
        aiMesh* currentMesh = aiScenePointer->mMeshes[n];
        // Skip points and lines ... for now -Ionut
        if (currentMesh->mNumVertices == 0) {
            continue;
        }

        Import::SubMeshData subMeshTemp;

        stringImpl name = currentMesh->mName.C_Str();
        if (Util::CompareIgnoreCase(name, "defaultobject")) {
            name.append("_" + fileName);
        }
        subMeshTemp.name(name);
        subMeshTemp.index(to_U32(n));
        if (subMeshTemp.name().empty()) {
            subMeshTemp.name(Util::StringFormat("%s-submesh-%d", fileName.c_str(), n));
        }
        if (subMeshTemp.name() == prevName) {
            subMeshTemp.name(prevName + "_" + to_stringImpl(n));
        }

        prevName = subMeshTemp.name();
        subMeshTemp.boneCount(currentMesh->mNumBones);
        loadSubMeshGeometry(currentMesh, 
                            subMeshTemp);

        loadSubMeshMaterial(subMeshTemp._material,
                            aiScenePointer,
                            to_U16(currentMesh->mMaterialIndex),
                            subMeshTemp.name() + "_material",
                            subMeshTemp.boneCount() > 0);
                            

        target._subMeshData.push_back(subMeshTemp);
    }

    size_t indexCount = 0;
    U32 vertexCount = 0;
    for (Import::SubMeshData& data : target._subMeshData) {
        indexCount += data._indices.size();
        vertexCount += to_U32(data._vertices.size());
    }

    target.vertexBuffer(context.gfx().newVB());
    VertexBuffer* vb = target.vertexBuffer();
    vb->useLargeIndices(vertexCount + 1 > std::numeric_limits<U16>::max());
    vb->setVertexCount(vertexCount);

    U8 submeshBoneOffset = 0;
    U32 previousOffset = 0;

    vec3<U32> triangleTemp;
    for (Import::SubMeshData& data : target._subMeshData) {
        BoundingBox importBB;
        size_t idxCount = data._indices.size();
        data._triangles.reserve(idxCount / 3);
        for (size_t i = 0; i < idxCount; i += 3) {
            for (U8 j = 0; j < 3; ++j) {
                triangleTemp[j] = data._indices[i + j];
                vb->addIndex(triangleTemp[j] + previousOffset);
            }
            data._triangles.push_back(triangleTemp);
        }
        
        U32 vertCount = to_U32(data._vertices.size());
        for (U32 i = 0; i < vertCount; ++i) {
            Import::SubMeshData::Vertex& vert = data._vertices[i];

            vb->modifyPositionValue(i + previousOffset, vert.position);
            vb->modifyNormalValue(i + previousOffset, vert.normal);

            if (!IS_ZERO(vert.texcoord.z)) {
                vb->modifyTexCoordValue(i + previousOffset, vert.texcoord.xy());
            }
            if (!IS_ZERO(vert.tangent.w)) {
                vb->modifyTangentValue(i + previousOffset, vert.tangent.xyz());
            }

            if (data.boneCount() > 0) {
                for (auto& idx : vert.indices.b) {
                    idx += submeshBoneOffset;
                }

                vb->modifyBoneIndices(i + previousOffset, vert.indices);
                vb->modifyBoneWeights(i + previousOffset, vert.weights);
            }

            importBB.add(vert.position);
        }
        data.minPos(importBB.getMin());
        data.maxPos(importBB.getMax());

        submeshBoneOffset += to_U8(data.boneCount());
        previousOffset += to_U32(data._vertices.size());
        data.partitionOffset(to_U32(vb->partitionBuffer()));
    }

    return true;
}

void DVDConverter::loadSubMeshGeometry(const aiMesh* source,
                                       Import::SubMeshData& subMeshData) {
    vectorEASTL<U32> input_indices;
    input_indices.reserve(source->mNumFaces * 3);
    for (U32 k = 0; k < source->mNumFaces; k++) {
        // guaranteed to be 3 thanks to aiProcess_Triangulate 
        for (U32 m = 0; m < 3; ++m) {
            input_indices.push_back(source->mFaces[k].mIndices[m]);
        }
    }
    subMeshData._indices.resize(input_indices.size());

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
                weightsPerVertex[bone->mWeights[b].mVertexId].push_back(vertexWeight(a, bone->mWeights[b].mWeight));
            }
        }

        vec4<F32> weights;
        P32       indices;
        for (U32 j = 0; j < source->mNumVertices; ++j) {
            indices.i = 0;
            weights.reset();
            // guaranteed to be max 4 thanks to aiProcess_LimitBoneWeights 
            for (U8 a = 0; a < weightsPerVertex[j].size(); ++a) {
                indices.b[a] = to_U8(weightsPerVertex[j][a]._boneID);
                weights[a] = weightsPerVertex[j][a]._boneWeight;
            }
            vertices[j].indices = indices;
            vertices[j].weights.set(weights);
        }
    }

    size_t vertex_count = 0;
    vectorEASTL<Import::SubMeshData::Vertex>& target_vertices = subMeshData._vertices;
    { //Remap VB & IB
        vectorEASTL<U32> remap(source->mNumVertices);
        vertex_count = meshopt_generateVertexRemap(&remap[0], input_indices.data(), input_indices.size(), &vertices[0], source->mNumVertices, sizeof(Import::SubMeshData::Vertex));

        meshopt_remapIndexBuffer(&subMeshData._indices[0], &input_indices[0], input_indices.size(), &remap[0]);
        input_indices.clear();

        target_vertices.resize(vertex_count);
        meshopt_remapVertexBuffer(&target_vertices[0], &vertices[0], source->mNumVertices, sizeof(Import::SubMeshData::Vertex), &remap[0]);
        vertices.clear();
        remap.clear();
    }
    { // Optimise VB & IB
        size_t index_count = subMeshData._indices.size();

        meshopt_optimizeVertexCache(&subMeshData._indices[0], &subMeshData._indices[0], index_count, vertex_count);

        // reorder indices for overdraw, balancing overdraw and vertex cache efficiency
        const F32 kThreshold = 1.01f; // allow up to 1% worse ACMR to get more reordering opportunities for overdraw
        meshopt_optimizeOverdraw(&subMeshData._indices[0], &subMeshData._indices[0], index_count, &target_vertices[0].position.x, target_vertices.size(), sizeof(Import::SubMeshData::Vertex), kThreshold);

        // vertex fetch optimization should go last as it depends on the final index order
        meshopt_optimizeVertexFetch(&target_vertices[0], &subMeshData._indices[0], index_count, &target_vertices[0], target_vertices.size(), sizeof(Import::SubMeshData::Vertex));
    }
    { // Generate LoD data and place inside VB & IB with proper offsets
        //ToDo: Create LoDs with meshoptimizer and store them all in the same buffers but with offsets
    }
}

void DVDConverter::loadSubMeshMaterial(Import::MaterialData& material,
                                       const aiScene* source,
                                       const U16 materialIndex,
                                       const stringImpl& materialName,
                                       bool skinned) {

    const aiMaterial* mat = source->mMaterials[materialIndex];

    material.name(materialName);
    Material::ColourData& data = material._colourData;

    // default shading model
    I32 shadingModel = to_base(Material::ShadingMode::PHONG);
    // Load shading model
    aiGetMaterialInteger(mat, AI_MATKEY_SHADING_MODEL, &shadingModel);
    material.shadingMode(aiShadingModeInternalTable[shadingModel]);

    bool isPBRMaterial = !(material.shadingMode() != Material::ShadingMode::OREN_NAYAR && material.shadingMode() != Material::ShadingMode::COOK_TORRANCE);
    
    // Load material opacity value
    F32 alpha = 1.0f;
    aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &alpha);

    // default diffuse colour
    data.baseColour(FColour4(0.8f, 0.8f, 0.8f, 1.0f));
    // Load diffuse colour
    aiColor4D diffuse;
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
        data.baseColour(FColour4(diffuse.r, diffuse.g, diffuse.b, alpha));
    } else {
        Console::d_printfn(Locale::get(_ID("MATERIAL_NO_DIFFUSE")), materialName.c_str());
    }

    // default emissive colour
    data.emissive(FColour3(0.0f, 0.0f, 0.0f));
    // Load emissive colour
    aiColor4D emissive;
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &emissive)) {
        data.emissive(FColour3(emissive.r, emissive.g, emissive.b));
    }


    // Ignore ambient colour
    vec4<F32> ambientTemp(0.0f, 0.0f, 0.0f, 1.0f);
    aiColor4D ambient;
    if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &ambient)) {
        ambientTemp.set(&ambient.r);
    } else {
        // no ambient
    }

    if (isPBRMaterial) {
        // Default shininess values
        F32 roughness = 0.0f, metallic = 0.0f;
        // Load shininess
        aiGetMaterialFloat(mat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &metallic);
        aiGetMaterialFloat(mat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &roughness);
        data.roughness(roughness);
        data.metallic(metallic);
    } else {
        // default specular colour
        data.specular(FColour3(1.0f, 1.0f, 1.0f));
        // Load specular colour
        aiColor4D specular;
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &specular)) {
            data.specular(FColour3(specular.r, specular.g, specular.b));
        } else {
            Console::d_printfn(Locale::get(_ID("MATERIAL_NO_SPECULAR")), materialName.c_str());
        }

        // Default shininess values
        F32 shininess = 15, strength = 1;
        // Load shininess
        aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &shininess);
        // Load shininess strength
        aiGetMaterialFloat(mat, AI_MATKEY_SHININESS_STRENGTH, &strength);
        F32 finalValue = shininess * strength;
        CLAMP<F32>(finalValue, 0.0f, 255.0f);
        data.shininess(finalValue);
    }

    // check if material is two sided
    I32 two_sided = 0;
    aiGetMaterialInteger(mat, AI_MATKEY_TWOSIDED, &two_sided);
    material.doubleSided(two_sided != 0);

    aiString tName;
    aiTextureMapping mapping;
    U32 uvInd;
    F32 blend;
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
        stringImpl path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.data);

        FileWithPath fileResult = splitPathToNameAndLocation(path);
        const stringImpl& img_name = fileResult._fileName;
        const stringImpl& img_path = fileResult._path;

        // if we have a name and an extension
        if (!img_name.substr(img_name.find_first_of(".")).empty()) {

            ShaderProgram::TextureUsage usage = count == 1 ? ShaderProgram::TextureUsage::UNIT1
                                                           : ShaderProgram::TextureUsage::UNIT0;

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
            texture.operation(count == 0 ? Material::TextureOperation::NONE
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

    result = mat->GetTexture(aiTextureType_NORMALS, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
    if (result == AI_SUCCESS) {
        stringImpl path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.data);

        FileWithPath fileResult = splitPathToNameAndLocation(path);
        const stringImpl& img_name = fileResult._fileName;
        const stringImpl& img_path = fileResult._path;

        Import::TextureEntry& texture = material._textures[to_base(ShaderProgram::TextureUsage::NORMALMAP)];

        if (img_name.rfind('.') != stringImpl::npos) {
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

            material._textures[to_base(ShaderProgram::TextureUsage::NORMALMAP)] = texture;
            material.bumpMethod(Material::BumpMethod::NORMAL);
        }  // endif
    } // endif

    result = mat->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping,&uvInd, &blend, &op, mode);

    if (result == AI_SUCCESS) {
        stringImpl path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.data);

        FileWithPath fileResult = splitPathToNameAndLocation(path);
        const stringImpl& img_name = fileResult._fileName;
        const stringImpl& img_path = fileResult._path;

        Import::TextureEntry& texture = material._textures[to_base(ShaderProgram::TextureUsage::NORMALMAP)];
        if (img_name.rfind('.') != stringImpl::npos) {
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
            material._textures[to_base(ShaderProgram::TextureUsage::HEIGHTMAP)] = texture;
        }  // endif
    } // endif

    I32 flags = 0;
    aiGetMaterialInteger(mat, AI_MATKEY_TEXFLAGS_DIFFUSE(0), &flags);
    material.ignoreAlpha((flags & aiTextureFlags_IgnoreAlpha) != 0);

    if (!material.ignoreAlpha()) {
        result = mat->GetTexture(aiTextureType_OPACITY, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
        if (result == AI_SUCCESS) {
            stringImpl path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.data);

            FileWithPath fileResult = splitPathToNameAndLocation(path);
            const stringImpl& img_name = fileResult._fileName;
            const stringImpl& img_path = fileResult._path;

            Import::TextureEntry& texture = material._textures[to_base(ShaderProgram::TextureUsage::OPACITY)];

            if (img_name.rfind('.') != stringImpl::npos) {
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
                material._textures[to_base(ShaderProgram::TextureUsage::OPACITY)] = texture;
            }  // endif
        } 
    }

    if (isPBRMaterial) {
        result = mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &tName, &mapping, &uvInd, &blend, &op, mode);
    } else {
        result = mat->GetTexture(aiTextureType_SPECULAR, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
    }
    if (result == AI_SUCCESS) {
        stringImpl path(Paths::g_assetsLocation + Paths::g_texturesLocation + tName.data);

        FileWithPath fileResult = splitPathToNameAndLocation(path);
        const stringImpl& img_name = fileResult._fileName;
        const stringImpl& img_path = fileResult._path;

        Import::TextureEntry& texture = material._textures[to_base(ShaderProgram::TextureUsage::SPECULAR)];
        if (img_name.rfind('.') != stringImpl::npos) {
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

            material._textures[to_base(ShaderProgram::TextureUsage::SPECULAR)] = texture;
        }
    }
}
};