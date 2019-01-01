#include "stdafx.h"

#include "Headers/MeshImporter.h"
#include "Headers/DVDConverter.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

namespace {
    const stringImpl g_parsedAssetGeometryExt("DVDGeom");
    const stringImpl g_parsedAssetAnimationExt("DVDAnim");
};

namespace Import {
    ImportData::~ImportData()
    {
    }

    bool ImportData::saveToFile(PlatformContext& context, const stringImpl& path, const stringImpl& fileName) {
        ACKNOWLEDGE_UNUSED(context);

        ByteBuffer tempBuffer;
        assert(_vertexBuffer != nullptr);
        tempBuffer << _ID("BufferEntryPoint");
        tempBuffer << _modelName;
        tempBuffer << _modelPath;
        if (_vertexBuffer->serialize(tempBuffer)) {
            tempBuffer << to_U32(_subMeshData.size());
            for (const SubMeshData& subMesh : _subMeshData) {
                if (!subMesh.serialize(tempBuffer)) {
                    //handle error
                }
            }
            tempBuffer << _hasAnimations;
            // Animations are handled by the SceneAnimator I/O
            return tempBuffer.dumpToFile(path, fileName + "." + g_parsedAssetGeometryExt);
        }

        return false;
    }

    bool ImportData::loadFromFile(PlatformContext& context, const stringImpl& path, const stringImpl& fileName) {
        ByteBuffer tempBuffer;
        if (tempBuffer.loadFromFile(path, fileName + "." + g_parsedAssetGeometryExt)) {
            U64 signature;
            tempBuffer >> signature;
            if (signature != _ID("BufferEntryPoint")) {
                return false;
            }
            tempBuffer >> _modelName;
            tempBuffer >> _modelPath;
            _vertexBuffer = context.gfx().newVB();
            if (_vertexBuffer->deserialize(tempBuffer)) {
                U32 subMeshCount = 0;
                tempBuffer >> subMeshCount;
                _subMeshData.resize(subMeshCount);
                for (SubMeshData& subMesh : _subMeshData) {
                    if (!subMesh.deserialize(tempBuffer)) {
                        //handle error
                        DIVIDE_UNEXPECTED_CALL();
                    }
                }
                tempBuffer >> _hasAnimations;
                _loadedFromFile = true;
                return true;
            }
        }
        return false;
    }

    bool SubMeshData::serialize(ByteBuffer& dataOut) const {
        dataOut << _name;
        dataOut << _index;
        dataOut << _boneCount;
        dataOut << _partitionOffset;
        dataOut << _minPos;
        dataOut << _maxPos;
        dataOut << _triangles;
        return _material.serialize(dataOut);
    }

    bool SubMeshData::deserialize(ByteBuffer& dataIn) {
        dataIn >> _name;
        dataIn >> _index;
        dataIn >> _boneCount;
        dataIn >> _partitionOffset;
        dataIn >> _minPos;
        dataIn >> _maxPos;
        dataIn >> _triangles;
        return _material.deserialize(dataIn);
    }

    bool MaterialData::serialize(ByteBuffer& dataOut) const {
        dataOut << _ignoreAlpha;
        dataOut << _doubleSided;
        dataOut << _name;
        dataOut << _colourData._diffuse;
        dataOut << _colourData._specular;
        dataOut << _colourData._emissive;
        dataOut << _colourData._shininess;
        dataOut << to_U32(_shadingMode);
        dataOut << to_U32(_bumpMethod);
        for (const TextureEntry& texture : _textures) {
            if (!texture.serialize(dataOut)) {
                //handle error
                DIVIDE_UNEXPECTED_CALL();
            }
        }

        return true;
    }

    bool MaterialData::deserialize(ByteBuffer& dataIn) {
        U32 temp = 0;
        dataIn >> _ignoreAlpha;
        dataIn >> _doubleSided;
        dataIn >> _name;
        dataIn >> _colourData._diffuse;
        dataIn >> _colourData._specular;
        dataIn >> _colourData._emissive;
        dataIn >> _colourData._shininess;
        dataIn >> temp;
        _shadingMode = static_cast<Material::ShadingMode>(temp);
        dataIn >> temp;
        _bumpMethod = static_cast<Material::BumpMethod>(temp);
        for (TextureEntry& texture : _textures) {
            if (!texture.deserialize(dataIn)) {
                //handle error
                DIVIDE_UNEXPECTED_CALL();
            }
        }

        return true;
    }

    bool TextureEntry::serialize(ByteBuffer& dataOut) const {
        dataOut << _textureName;
        dataOut << _texturePath;
        dataOut << _srgbSpace;
        dataOut << to_U32(_wrapU);
        dataOut << to_U32(_wrapV);
        dataOut << to_U32(_wrapW);
        dataOut << to_U32(_operation);
        return true;
    }

    bool TextureEntry::deserialize(ByteBuffer& dataIn) {
        U32 data = 0;
        dataIn >> _textureName;
        dataIn >> _texturePath;
        dataIn >> _srgbSpace;
        dataIn >> data;
        _wrapU = static_cast<TextureWrap>(data);
        dataIn >> data;
        _wrapV = static_cast<TextureWrap>(data);
        dataIn >> data;
        _wrapW = static_cast<TextureWrap>(data);
        dataIn >> data;
        _operation = static_cast<Material::TextureOperation>(data);
        return true;
    }
};
    bool MeshImporter::loadMeshDataFromFile(PlatformContext& context, ResourceCache& cache, Import::ImportData& dataOut) {
        Time::ProfileTimer importTimer;
        importTimer.start();

        stringImpl modelName = dataOut._modelPath.substr(dataOut._modelPath.find_last_of('/') + 1);
        stringImpl path = dataOut._modelPath.substr(0, dataOut._modelPath.find_last_of('/'));

        bool success = false;
        if (!dataOut.loadFromFile(context, Paths::g_cacheLocation + Paths::g_geometryCacheLocation, modelName)) {
            Console::printfn(Locale::get(_ID("MESH_NOT_LOADED_FROM_FILE")), modelName.c_str());

            DVDConverter converter(context, dataOut, dataOut._modelPath, success);
            if (success) {
                dataOut._modelName = modelName;
                dataOut._modelPath = path;
                if (dataOut.saveToFile(context, Paths::g_cacheLocation + Paths::g_geometryCacheLocation, modelName)) {
                    Console::printfn(Locale::get(_ID("MESH_SAVED_TO_FILE")), modelName.c_str());
                } else {
                    Console::printfn(Locale::get(_ID("MESH_NOT_SAVED_TO_FILE")), modelName.c_str());
                }
            }
        } else {
            Console::printfn(Locale::get(_ID("MESH_LOADED_FROM_FILE")), modelName.c_str());
            success = true;
        }

        importTimer.stop();
        Console::d_printfn(Locale::get(_ID("LOAD_MESH_TIME")),
                           modelName.c_str(),
                           Time::MicrosecondsToSeconds<F32>(importTimer.get()));

        return success;
    }

    bool MeshImporter::loadMesh(Mesh_ptr mesh, PlatformContext& context, ResourceCache& cache, const Import::ImportData& dataIn) {
        Time::ProfileTimer importTimer;
        importTimer.start();

        std::shared_ptr<SceneAnimator> animator;
        if (dataIn._hasAnimations) {
            mesh->setObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);

            ByteBuffer tempBuffer;
            animator.reset(new SceneAnimator());
            if (tempBuffer.loadFromFile(Paths::g_cacheLocation + Paths::g_geometryCacheLocation,
                                        dataIn._modelName + "." + g_parsedAssetAnimationExt))
            {
                animator->load(context, tempBuffer);
            } else {
                if (!dataIn._loadedFromFile) {
                    Attorney::SceneAnimatorMeshImporter::registerAnimations(*animator, dataIn._animations);
                    animator->init(context, dataIn._skeleton, dataIn._bones);
                    animator->save(context, tempBuffer);
                    if (!tempBuffer.dumpToFile(Paths::g_cacheLocation + Paths::g_geometryCacheLocation,
                                               dataIn._modelName + "." + g_parsedAssetAnimationExt))
                    {
                        //handle error
                        DIVIDE_UNEXPECTED_CALL();
                    }
                } else {
                    //handle error. No ASSIMP animation data available
                    DIVIDE_UNEXPECTED_CALL();
                }
            }
            mesh->setAnimator(animator);
            animator = nullptr;
        }

        mesh->renderState().setDrawState(true);
        mesh->getGeometryVB()->fromBuffer(*dataIn._vertexBuffer);
        mesh->setGeometryVBDirty();

        SubMesh_ptr tempSubMesh;
        for (const Import::SubMeshData& subMeshData : dataIn._subMeshData) {
            // Submesh is created as a resource when added to the scenegraph
            ResourceDescriptor submeshdesc(subMeshData._name);
            submeshdesc.setFlag(true);
            submeshdesc.setID(subMeshData._index);
            if (subMeshData._boneCount > 0) {
                submeshdesc.setEnumValue(to_base(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
            }

            tempSubMesh = CreateResource<SubMesh>(cache, submeshdesc);
            if (!tempSubMesh) {
                continue;
            }
            // it may already be loaded
            if (!tempSubMesh->getParentMesh()) {
                tempSubMesh->addTriangles(subMeshData._triangles);
                tempSubMesh->setGeometryPartitionID(subMeshData._partitionOffset);
                Attorney::SubMeshMeshImporter::setGeometryLimits(*tempSubMesh,
                                                                 subMeshData._minPos,
                                                                 subMeshData._maxPos);

                if (!tempSubMesh->getMaterialTpl()) {
                    tempSubMesh->setMaterialTpl(loadSubMeshMaterial(context, cache, subMeshData._material, subMeshData._boneCount > 0));
                }
            }

            mesh->addSubMesh(tempSubMesh);
        }

        mesh->getGeometryVB()->create();
        mesh->getGeometryVB()->keepData(mesh->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));

        importTimer.stop();
        Console::d_printfn(Locale::get(_ID("PARSE_MESH_TIME")),
                           dataIn._modelName.c_str(),
                           Time::MillisecondsToSeconds(importTimer.get()));

        return true;
    }

    /// Load the material for the current SubMesh
    Material_ptr MeshImporter::loadSubMeshMaterial(PlatformContext& context, ResourceCache& cache, const Import::MaterialData& importData, bool skinned) {
        ResourceDescriptor materialDesc(importData._name);
        if (skinned) {
            materialDesc.setEnumValue(to_base(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
        }

        bool wasInCache = false;
        Material_ptr tempMaterial = CreateResource<Material>(cache, materialDesc, wasInCache);
        if (wasInCache) {
            return tempMaterial;
        }
        tempMaterial->setColourData(importData._colourData);
        tempMaterial->setShadingMode(importData._shadingMode);
        tempMaterial->setBumpMethod(importData._bumpMethod);
        tempMaterial->setDoubleSided(importData._doubleSided);

        tempMaterial->useTriangleStrip(false);
        SamplerDescriptor textureSampler = {};

        TextureDescriptor textureDescriptor(TextureType::TEXTURE_2D);

        for (U32 i = 0; i < to_base(ShaderProgram::TextureUsage::COUNT); ++i) {
            const Import::TextureEntry& tex = importData._textures[i];
            if (!tex._textureName.empty()) {
                textureSampler._wrapU = tex._wrapU;
                textureSampler._wrapV = tex._wrapV;
                textureSampler._wrapW = tex._wrapW;

                textureDescriptor.setSampler(textureSampler);
                textureDescriptor._srgb = tex._srgbSpace;

                ResourceDescriptor texture(tex._textureName);
                texture.assetName(tex._textureName);
                texture.assetLocation(tex._texturePath);
                texture.setPropertyDescriptor(textureDescriptor);
                
                Texture_ptr textureRes = CreateResource<Texture>(cache, texture);
                assert(textureRes != nullptr);

                tempMaterial->setTexture(static_cast<ShaderProgram::TextureUsage>(i), textureRes, tex._operation);
            }
        }

        // If we don't have a valid opacity map, try to find out whether the diffuse texture has any non-opaque pixels.
        // If we find a few, use it as opacity texture
        if (!importData._ignoreAlpha && importData._textures[to_base(ShaderProgram::TextureUsage::OPACITY)]._textureName.empty()) {
            Texture_ptr diffuse = tempMaterial->getTexture(ShaderProgram::TextureUsage::UNIT0).lock();
            if (diffuse && diffuse->hasTransparency()) {
                ResourceDescriptor opacityDesc(diffuse->resourceName());
                //These should not be needed. We should be able to just find the resource in cache!
                opacityDesc.assetName(diffuse->assetName());
                opacityDesc.assetLocation(diffuse->assetLocation());
                opacityDesc.setPropertyDescriptor(diffuse->getDescriptor());

                Texture_ptr textureRes = CreateResource<Texture>(cache, opacityDesc);
                tempMaterial->setTexture(ShaderProgram::TextureUsage::OPACITY, textureRes, Material::TextureOperation::REPLACE);
            }
        }
        
        return tempMaterial;
    }
}; //namespace Divide