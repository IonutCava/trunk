#include "stdafx.h"

#include "Headers/MeshImporter.h"
#include "Headers/DVDConverter.h"

#include "Core/Headers/Configuration.h"
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
    const char* g_parsedAssetGeometryExt = "DVDGeom";
    const char* g_parsedAssetAnimationExt = "DVDAnim";
};

namespace Import {
    ImportData::~ImportData()
    {
    }

    bool ImportData::saveToFile(PlatformContext& context, const Str256& path, const Str64& fileName) {
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
            return tempBuffer.dumpToFile(path.c_str(), (fileName + "." + g_parsedAssetGeometryExt).c_str());
        }

        return false;
    }

    bool ImportData::loadFromFile(PlatformContext& context, const Str256& path, const Str64& fileName) {
        ByteBuffer tempBuffer;
        if (tempBuffer.loadFromFile(path.c_str(), (fileName + "." + g_parsedAssetGeometryExt).c_str())) {
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
        dataOut << _partitionIDs;
        dataOut << _minPos;
        dataOut << _maxPos;
        for (U8 i = 0; i < MAX_LOD_LEVELS; ++i) {
            dataOut << _triangles[i];
        }
        return _material.serialize(dataOut);
    }

    bool SubMeshData::deserialize(ByteBuffer& dataIn) {
        dataIn >> _name;
        dataIn >> _index;
        dataIn >> _boneCount;
        dataIn >> _partitionIDs;
        dataIn >> _minPos;
        dataIn >> _maxPos;
        for (U8 i = 0; i < MAX_LOD_LEVELS; ++i) {
            dataIn >> _triangles[i];
        }
        return _material.deserialize(dataIn);
    }

    bool MaterialData::serialize(ByteBuffer& dataOut) const {
        dataOut << _ignoreAlpha;
        dataOut << _doubleSided;
        dataOut << _name;
        dataOut << baseColour();
        dataOut << emissive();
        dataOut << CLAMPED_01(metallic());
        dataOut << CLAMPED_01(roughness());
        dataOut << CLAMPED_01(parallaxFactor());
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
        dataIn >> _ignoreAlpha;
        dataIn >> _doubleSided;
        dataIn >> _name;
        FColour4 tempBase = {};
        dataIn >> tempBase;
        baseColour(tempBase);
        FColour3 tempEmissive = {};
        dataIn >> tempEmissive;
        emissive(tempEmissive);
        F32 temp = {};
        dataIn >> temp;
        metallic(temp);
        dataIn >> temp;
        roughness(temp);
        dataIn >> temp;
        parallaxFactor(temp);
        U32 temp2 = {};
        dataIn >> temp2;
        _shadingMode = static_cast<ShadingMode>(temp2);
        dataIn >> temp2;
        _bumpMethod = static_cast<BumpMethod>(temp2);
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
        dataOut << _srgb;
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
        dataIn >> _srgb;
        dataIn >> data;
        _wrapU = static_cast<TextureWrap>(data);
        dataIn >> data;
        _wrapV = static_cast<TextureWrap>(data);
        dataIn >> data;
        _wrapW = static_cast<TextureWrap>(data);
        dataIn >> data;
        _operation = static_cast<TextureOperation>(data);
        return true;
    }
};
    bool MeshImporter::loadMeshDataFromFile(PlatformContext& context, ResourceCache* cache, Import::ImportData& dataOut) {
        Time::ProfileTimer importTimer = {};
        importTimer.start();

        bool success = false;
        if (!context.config().debug.useGeometryCache || !dataOut.loadFromFile(context, Paths::g_cacheLocation + Paths::g_geometryCacheLocation, dataOut.modelName())) {
            Console::printfn(Locale::get(_ID("MESH_NOT_LOADED_FROM_FILE")), dataOut.modelName().c_str());

            DVDConverter converter(context, dataOut, success);
            if (success) {
                if (dataOut.saveToFile(context, Paths::g_cacheLocation + Paths::g_geometryCacheLocation, dataOut.modelName())) {
                    Console::printfn(Locale::get(_ID("MESH_SAVED_TO_FILE")), dataOut.modelName().c_str());
                } else {
                    Console::printfn(Locale::get(_ID("MESH_NOT_SAVED_TO_FILE")), dataOut.modelName().c_str());
                }
            }
        } else {
            Console::printfn(Locale::get(_ID("MESH_LOADED_FROM_FILE")), dataOut.modelName().c_str());
            success = true;
        }

        importTimer.stop();
        Console::d_printfn(Locale::get(_ID("LOAD_MESH_TIME")),
                           dataOut.modelName().c_str(),
                           Time::MicrosecondsToMilliseconds<F32>(importTimer.get()));

        return success;
    }

    bool MeshImporter::loadMesh(Mesh_ptr mesh, PlatformContext& context, ResourceCache* cache, const Import::ImportData& dataIn) {
        Time::ProfileTimer importTimer;
        importTimer.start();

        std::shared_ptr<SceneAnimator> animator;
        if (dataIn.hasAnimations()) {
            mesh->setObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);

            ByteBuffer tempBuffer;
            animator.reset(new SceneAnimator());
            if (tempBuffer.loadFromFile((Paths::g_cacheLocation + Paths::g_geometryCacheLocation).c_str(),
                                        (dataIn.modelName() + "." + g_parsedAssetAnimationExt).c_str()))
            {
                animator->load(context, tempBuffer);
            } else {
                if (!dataIn.loadedFromFile()) {
                    Attorney::SceneAnimatorMeshImporter::registerAnimations(*animator, dataIn._animations);
                    animator->init(context, dataIn.skeleton(), dataIn._bones);
                    animator->save(context, tempBuffer);
                    if (!tempBuffer.dumpToFile((Paths::g_cacheLocation + Paths::g_geometryCacheLocation).c_str(),
                                               (dataIn.modelName() + "." + g_parsedAssetAnimationExt).c_str()))
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

        mesh->renderState().drawState(true);
        mesh->getGeometryVB()->fromBuffer(*dataIn.vertexBuffer());
        mesh->setGeometryVBDirty();

        SubMesh_ptr tempSubMesh;
        for (const Import::SubMeshData& subMeshData : dataIn._subMeshData) {
            // Submesh is created as a resource when added to the scenegraph
            ResourceDescriptor submeshdesc(subMeshData.name());
            submeshdesc.flag(true);
            submeshdesc.ID(subMeshData.index());
            if (subMeshData.boneCount() > 0) {
                submeshdesc.enumValue(to_base(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
            }

            tempSubMesh = CreateResource<SubMesh>(cache, submeshdesc);
            if (!tempSubMesh) {
                continue;
            }
            // it may already be loaded
            if (!tempSubMesh->getParentMesh()) {
                tempSubMesh->addTriangles(subMeshData._triangles[0]);
                for (U8 i = 0; i < Import::MAX_LOD_LEVELS; ++i) {
                    tempSubMesh->setGeometryPartitionID(i, subMeshData._partitionIDs[i]);
                }
                Attorney::SubMeshMeshImporter::setGeometryLimits(*tempSubMesh,
                                                                 subMeshData.minPos(),
                                                                 subMeshData.maxPos());

                if (!tempSubMesh->getMaterialTpl()) {
                    tempSubMesh->setMaterialTpl(loadSubMeshMaterial(context, cache, subMeshData._material, subMeshData.boneCount() > 0));
                }
            }

            mesh->addSubMesh(tempSubMesh);
        }

        mesh->getGeometryVB()->create();
        mesh->getGeometryVB()->keepData(mesh->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
        mesh->postImport();

        importTimer.stop();
        Console::d_printfn(Locale::get(_ID("PARSE_MESH_TIME")),
                           dataIn.modelName().c_str(),
                           Time::MicrosecondsToMilliseconds(importTimer.get()));

        return true;
    }

    /// Load the material for the current SubMesh
    Material_ptr MeshImporter::loadSubMeshMaterial(PlatformContext& context, ResourceCache* cache, const Import::MaterialData& importData, bool skinned) {
        ResourceDescriptor materialDesc(importData.name());
        if (skinned) {
            materialDesc.enumValue(to_base(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
        }

        bool wasInCache = false;
        Material_ptr tempMaterial = CreateResource<Material>(cache, materialDesc, wasInCache);
        if (wasInCache) {
            return tempMaterial;
        }
        tempMaterial->baseColour(importData.baseColour());
        tempMaterial->emissiveColour(importData.emissive());
        tempMaterial->metallic(importData.metallic());
        tempMaterial->roughness(importData.roughness());
        tempMaterial->parallaxFactor(importData.parallaxFactor());

        tempMaterial->shadingMode(importData.shadingMode());
        tempMaterial->bumpMethod(importData.bumpMethod());
        tempMaterial->doubleSided(importData.doubleSided());

        SamplerDescriptor textureSampler = {};

        TextureDescriptor textureDescriptor(TextureType::TEXTURE_2D);

        for (U32 i = 0; i < to_base(TextureUsage::COUNT); ++i) {
            const Import::TextureEntry& tex = importData._textures[i];
            if (!tex.textureName().empty()) {
                textureSampler.wrapU(tex.wrapU());
                textureSampler.wrapV(tex.wrapV());
                textureSampler.wrapW(tex.wrapW());

                textureDescriptor.srgb(tex.srgb());

                ResourceDescriptor texture(tex.textureName());
                texture.assetName(tex.textureName());
                texture.assetLocation(tex.texturePath());
                texture.propertyDescriptor(textureDescriptor);
                Texture_ptr texPtr = CreateResource<Texture>(cache, texture);
                texPtr->addStateCallback(ResourceState::RES_LOADED, [&](CachedResource* res) {
                    tempMaterial->setTexture(static_cast<TextureUsage>(i), texPtr, textureSampler.getHash(),tex.operation());
                });
            }
        }

        // If we don't have a valid opacity map, try to find out whether the diffuse texture has any non-opaque pixels.
        // If we find a few, use it as opacity texture
        if (!importData.ignoreAlpha() && importData._textures[to_base(TextureUsage::OPACITY)].textureName().empty()) {
            Texture_ptr diffuse = tempMaterial->getTexture(TextureUsage::UNIT0).lock();
            if (diffuse && diffuse->hasTransparency()) {
                ResourceDescriptor opacityDesc(diffuse->resourceName());
                //These should not be needed. We should be able to just find the resource in cache!
                opacityDesc.assetName(diffuse->assetName());
                opacityDesc.assetLocation(diffuse->assetLocation());
                opacityDesc.propertyDescriptor(diffuse->descriptor());
                Texture_ptr texPtr = CreateResource<Texture>(cache, opacityDesc);
                texPtr->addStateCallback(ResourceState::RES_LOADED, [&](CachedResource* res) {
                    tempMaterial->setTexture(TextureUsage::OPACITY, texPtr, tempMaterial->getSampler(TextureUsage::UNIT0),TextureOperation::REPLACE);
                });
                
            }
        }

        return tempMaterial;
    }
}; //namespace Divide