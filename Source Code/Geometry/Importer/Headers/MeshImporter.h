/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _MESH_IMPORTER_H_
#define _MESH_IMPORTER_H_

#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"

namespace Divide {
    class PlatformContext;
    class ByteBuffer;
    class VertexBuffer;
    namespace Import {
        struct TextureEntry {
            bool serialize(ByteBuffer& dataOut) const;
            bool deserialize(ByteBuffer& dataIn);

            PROPERTY_RW(stringImpl, textureName);
            PROPERTY_RW(stringImpl, texturePath);

            // Only Albedo/Diffuse should be sRGB
            // Normals, specular, etc should be in linear space
            PROPERTY_RW(bool, srgb, false);
            PROPERTY_RW(TextureWrap, wrapU, TextureWrap::REPEAT);
            PROPERTY_RW(TextureWrap, wrapV, TextureWrap::REPEAT);
            PROPERTY_RW(TextureWrap, wrapW, TextureWrap::REPEAT);
            PROPERTY_RW(Material::TextureOperation, operation, Material::TextureOperation::NONE);
        };

        struct MaterialData {
            bool serialize(ByteBuffer& dataOut) const;
            bool deserialize(ByteBuffer& dataIn);

            PROPERTY_RW(bool, ignoreAlpha, false);
            PROPERTY_RW(bool, doubleSided, true);
            PROPERTY_RW(stringImpl, name);
            PROPERTY_RW(Material::ShadingMode, shadingMode, Material::ShadingMode::FLAT);
            PROPERTY_RW(Material::BumpMethod,  bumpMethod, Material::BumpMethod::NONE);

            Material::ColourData _colourData;
            std::array<TextureEntry, to_base(ShaderProgram::TextureUsage::COUNT)> _textures;
        };

        struct SubMeshData {
            struct Vertex {
                vec3<F32> position = {0.f, 0.f, 0.f };
                vec3<F32> normal = {0.f, 0.f, 0.f };
                vec4<F32> tangent = { 0.f, 0.f, 0.f, 0.f };
                vec3<F32> texcoord = { 0.f, 0.f, 0.f };
                vec4<F32> weights;
                P32       indices;
            };

            bool serialize(ByteBuffer& dataOut) const;
            bool deserialize(ByteBuffer& dataIn);

            PROPERTY_RW(stringImpl, name);
            PROPERTY_RW(U32, index, 0u);
            PROPERTY_RW(U32, boneCount, 0u);
            PROPERTY_RW(U32, partitionOffset, 0u);
            PROPERTY_RW(vec3<F32>, minPos);
            PROPERTY_RW(vec3<F32>, maxPos);

            vectorEASTL<vec3<U32>> _triangles;
            vectorEASTL<U32> _indices;
            vectorEASTL<Vertex> _vertices;

            MaterialData _material;
        };

        struct ImportData {
            ImportData(const stringImpl& modelPath, const stringImpl& modelName)
                : _modelPath(modelPath),
                  _modelName(modelName)
            {
                _vertexBuffer = nullptr;
                _hasAnimations = false;
                _skeleton = nullptr;
                _loadedFromFile = false;
            }
            ~ImportData();

            bool saveToFile(PlatformContext& context, const stringImpl& path, const stringImpl& fileName);
            bool loadFromFile(PlatformContext& context, const stringImpl& path, const stringImpl& fileName);

            // Was it loaded from file, or just created?
            PROPERTY_RW(bool, loadedFromFile, false);
            // Geometry
            POINTER_RW(VertexBuffer, vertexBuffer, nullptr);
            // Animations
            POINTER_RW(Bone, skeleton, nullptr);
            PROPERTY_RW(bool, hasAnimations, false);

            // Name and path
            PROPERTY_RW(stringImpl, modelName);
            PROPERTY_RW(stringImpl, modelPath);

            vector<Bone*> _bones;
            vector<SubMeshData> _subMeshData;
            vector<std::shared_ptr<AnimEvaluator>> _animations;
        };
    };


    FWD_DECLARE_MANAGED_CLASS(Mesh);
    FWD_DECLARE_MANAGED_CLASS(Material);

    class MeshImporter
    {
        public:
            static bool loadMeshDataFromFile(PlatformContext& context, ResourceCache& cache, Import::ImportData& dataOut);
            static bool loadMesh(Mesh_ptr mesh, PlatformContext& context, ResourceCache& cache, const Import::ImportData& dataIn);

        protected:
            static Material_ptr loadSubMeshMaterial(PlatformContext& context, ResourceCache& cache, const Import::MaterialData& importData, bool skinned);
    };

};  // namespace Divide

#endif