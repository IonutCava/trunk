#include "Headers/MeshImporter.h"
#include "Headers/DVDConverter.h"

#include "Core/Headers/Console.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    const stringImpl g_parsedAssetLocation("parsed");
    const stringImpl g_parsedAssetGeometryExt("DVDGeom");
    const stringImpl g_parsedAssetAnimationExt("DVDAnim");
};

namespace Import {
    ImportData::~ImportData()
    {
        MemoryManager::SAFE_DELETE(_vertexBuffer);
    }

    bool ImportData::saveToFile(const stringImpl& fileName) {
        ByteBuffer tempBuffer;
        assert(_vertexBuffer != nullptr);
        tempBuffer << stringImpl("BufferEntryPoint");
        tempBuffer << _modelName;
        tempBuffer << _modelPath;
        _vertexBuffer->serialize(tempBuffer);
        tempBuffer << to_uint(_subMeshData.size());
        for (const SubMeshData& subMesh : _subMeshData) {
            if (!subMesh.serialize(tempBuffer)) {
                //handle error
            }
        }
        tempBuffer << _hasAnimations;
        // Animations are handled by the SceneAnimator I/O
        return tempBuffer.dumpToFile(fileName + "." + g_parsedAssetGeometryExt);
    }

    bool ImportData::loadFromFile(const stringImpl& fileName) {
        ByteBuffer tempBuffer;
        if (tempBuffer.loadFromFile(fileName + "." + g_parsedAssetGeometryExt)) {
            stringImpl signature;
            tempBuffer >> signature;
            if (signature.compare("BufferEntryPoint") != 0) {
                return false;
            }
            tempBuffer >> _modelName;
            tempBuffer >> _modelPath;
            _vertexBuffer = GFX_DEVICE.newVB();
            _vertexBuffer->deserialize(tempBuffer);
            U32 subMeshCount = 0;
            tempBuffer >> subMeshCount;
            _subMeshData.resize(subMeshCount);
            for (SubMeshData& subMesh : _subMeshData) {
                if (!subMesh.deserialize(tempBuffer)) {
                    //handle error
                    assert(false);
                }
            }
            tempBuffer >> _hasAnimations;
            _loadedFromFile = true;
            return true;
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
        dataOut << _shadingData._diffuse;
        dataOut << _shadingData._specular;
        dataOut << _shadingData._emissive;
        dataOut << _shadingData._shininess;
        dataOut << _shadingData._textureCount;
        dataOut << to_uint(_shadingMode);
        dataOut << to_uint(_bumpMethod);
        for (const TextureEntry& texture : _textures) {
            if (!texture.serialize(dataOut)) {
                //handle error
                assert(false);
            }
        }

        return true;
    }

    bool MaterialData::deserialize(ByteBuffer& dataIn) {
        U32 temp = 0;
        dataIn >> _ignoreAlpha;
        dataIn >> _doubleSided;
        dataIn >> _name;
        dataIn >> _shadingData._diffuse;
        dataIn >> _shadingData._specular;
        dataIn >> _shadingData._emissive;
        dataIn >> _shadingData._shininess;
        dataIn >> _shadingData._textureCount;
        dataIn >> temp;
        _shadingMode = static_cast<Material::ShadingMode>(temp);
        dataIn >> temp;
        _bumpMethod = static_cast<Material::BumpMethod>(temp);
        for (TextureEntry& texture : _textures) {
            if (!texture.deserialize(dataIn)) {
                //handle error
                assert(false);
            }
        }

        return true;
    }

    bool TextureEntry::serialize(ByteBuffer& dataOut) const {
        dataOut << _textureName;
        dataOut << _texturePath;
        dataOut << _srgbSpace;
        dataOut << to_uint(_wrapU);
        dataOut << to_uint(_wrapV);
        dataOut << to_uint(_wrapW);
        dataOut << to_uint(_operation);
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
    bool MeshImporter::loadMeshDataFromFile(Import::ImportData& dataOut) {
        Time::ProfileTimer importTimer;
        importTimer.start();

        stringImpl modelName = dataOut._modelPath.substr(dataOut._modelPath.find_last_of('/') + 1);
        stringImpl path = dataOut._modelPath.substr(0, dataOut._modelPath.find_last_of('/'));

        bool success = false;
        if (!dataOut.loadFromFile(path + "/" + g_parsedAssetLocation + "/" + modelName)) {
            Console::printfn(Locale::get(_ID("MESH_NOT_LOADED_FROM_FILE")), modelName.c_str());

            DVDConverter converter(dataOut, dataOut._modelPath, success);
            if (success) {
                dataOut._modelName = modelName;
                dataOut._modelPath = path;
                if (dataOut.saveToFile(path + "/" + g_parsedAssetLocation + "/" + modelName)) {
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
                           Time::MillisecondsToSeconds(importTimer.get()));

        return success;
    }

    Mesh_ptr MeshImporter::loadMesh(const stringImpl& name, const Import::ImportData& dataIn) {
        Time::ProfileTimer importTimer;
        importTimer.start();

        std::shared_ptr<SceneAnimator> animator;
        if (dataIn._hasAnimations) {
            ByteBuffer tempBuffer;
            animator.reset(new SceneAnimator());
            if (tempBuffer.loadFromFile(dataIn._modelPath + "/" + 
                                        g_parsedAssetLocation + "/" +
                                        dataIn._modelName + "." +
                                        g_parsedAssetAnimationExt)) {
                animator->load(tempBuffer);
            } else {
                if (!dataIn._loadedFromFile) {
                    for (const std::shared_ptr<AnimEvaluator>& animation : dataIn._animations) {
                        animator->registerAnimation(animation);
                    }
                    animator->init(dataIn._skeleton, dataIn._bones);
                    animator->save(tempBuffer);
                    if (!tempBuffer.dumpToFile(dataIn._modelPath + "/" + 
                                               g_parsedAssetLocation + "/" +
                                               dataIn._modelName + "." +
                                               g_parsedAssetAnimationExt)) {
                        //handle error
                        assert(false);
                    }
                } else {
                    //handle error. No ASSIMP animation data available
                    assert(false);
                }
            }
        }

        Mesh_ptr mesh(MemoryManager_NEW Mesh(name,
                                             dataIn._modelPath,
                                             dataIn._hasAnimations
                                                 ? Object3D::ObjectFlag::OBJECT_FLAG_SKINNED
                                                 : Object3D::ObjectFlag::OBJECT_FLAG_NONE),
                                DeleteResource());
        if (dataIn._hasAnimations) {
            mesh->setAnimator(animator);
            animator = nullptr;
        }

        mesh->renderState().setDrawState(true);
        mesh->getGeometryVB()->fromBuffer(*dataIn._vertexBuffer);

        SubMesh_ptr tempSubMesh;
        for (const Import::SubMeshData& subMeshData : dataIn._subMeshData) {
            // Submesh is created as a resource when added to the scenegraph
            ResourceDescriptor submeshdesc(subMeshData._name);
            submeshdesc.setFlag(true);
            submeshdesc.setID(subMeshData._index);
            if (subMeshData._boneCount > 0) {
                submeshdesc.setEnumValue(to_const_uint(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
            }

            tempSubMesh = CreateResource<SubMesh>(submeshdesc);
            if (!tempSubMesh) {
                continue;
            }
            // it may already be loaded
            if (!tempSubMesh->getParentMesh()) {
                for (const vec3<U32>& triangle : subMeshData._triangles) {
                    tempSubMesh->addTriangle(triangle);
                }
                tempSubMesh->setGeometryPartitionID(subMeshData._partitionOffset);
                Attorney::SubMeshMeshImporter::setGeometryLimits(*tempSubMesh,
                                                                 subMeshData._minPos,
                                                                 subMeshData._maxPos);

                if (!tempSubMesh->getMaterialTpl()) {
                    tempSubMesh->setMaterialTpl(loadSubMeshMaterial(subMeshData._material, subMeshData._boneCount > 0));
                }
            }

            mesh->addSubMesh(tempSubMesh);
        }

        mesh->getGeometryVB()->create(!mesh->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
        
        importTimer.stop();
        Console::d_printfn(Locale::get(_ID("PARSE_MESH_TIME")),
                           dataIn._modelName.c_str(),
                           Time::MillisecondsToSeconds(importTimer.get()));

        return mesh;
    }

    /// Load the material for the current SubMesh
    Material_ptr MeshImporter::loadSubMeshMaterial(const Import::MaterialData& importData, bool skinned) {
        // See if the material already exists in a cooked state (XML file)
        STUBBED("LOADING MATERIALS FROM XML IS DISABLED FOR NOW! - Ionut")
#if !defined(DEBUG)
            const bool DISABLE_MAT_FROM_FILE = true;
#else
            const bool DISABLE_MAT_FROM_FILE = true;
#endif

        Material_ptr tempMaterial;
        if (!DISABLE_MAT_FROM_FILE) {
            tempMaterial = XML::loadMaterial(importData._name);
            if (tempMaterial) {
                return tempMaterial;
            }
        }

        // If it's not defined in an XML File, see if it was previously loaded by
        // the Resource Cache
        tempMaterial = FindResourceImpl<Material>(importData._name);
        if (tempMaterial) {
            return tempMaterial;
        }

        // If we found it in the Resource Cache, return a copy of it
        ResourceDescriptor materialDesc(importData._name);
        if (skinned) {
            materialDesc.setEnumValue(
                to_const_uint(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED));
        }

        tempMaterial = CreateResource<Material>(materialDesc);

        tempMaterial->setShaderData(importData._shadingData);
        tempMaterial->setShadingMode(importData._shadingMode);
        tempMaterial->setBumpMethod(importData._bumpMethod);
        tempMaterial->setDoubleSided(importData._doubleSided);

        SamplerDescriptor textureSampler;
        for (U32 i = 0; i < to_const_uint(ShaderProgram::TextureUsage::COUNT); ++i) {
            const Import::TextureEntry& tex = importData._textures[i];
            if (!tex._textureName.empty()) {
                textureSampler.toggleSRGBColorSpace(tex._srgbSpace);
                textureSampler.setWrapMode(tex._wrapU, tex._wrapV, tex._wrapW);

                ResourceDescriptor texture(tex._textureName);
                texture.setResourceLocation(tex._texturePath);
                texture.setPropertyDescriptor<SamplerDescriptor>(textureSampler);
                texture.setEnumValue(to_const_uint(TextureType::TEXTURE_2D));
                Texture_ptr textureRes = CreateResource<Texture>(texture);
                assert(textureRes != nullptr);

                tempMaterial->setTexture(static_cast<ShaderProgram::TextureUsage>(i), textureRes, tex._operation);
            }
        }

        // If we don't have a valid opacity map, try to find out whether the diffuse texture has any non-opaque pixels.
        // If we find a few, use it as opacity texture
        if (!importData._ignoreAlpha && importData._textures[to_const_uint(ShaderProgram::TextureUsage::OPACITY)]._textureName.empty()) {
            Texture_ptr diffuse = tempMaterial->getTexture(ShaderProgram::TextureUsage::UNIT0).lock();
            if (diffuse && diffuse->hasTransparency()) {
                Texture_ptr textureRes = CreateResource<Texture>(ResourceDescriptor(diffuse->getName()));
                tempMaterial->setTexture(ShaderProgram::TextureUsage::OPACITY, textureRes, Material::TextureOperation::REPLACE);
            }
        }
        
        return tempMaterial;
    }
}; //namespace Divide