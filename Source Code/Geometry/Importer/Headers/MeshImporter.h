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
            TextureEntry()
              : _srgbSpace(false),
                _wrapU(TextureWrap::REPEAT),
                _wrapV(TextureWrap::REPEAT),
                _wrapW(TextureWrap::REPEAT),
                _operation(Material::TextureOperation::REPLACE)
            {
            }

            bool serialize(ByteBuffer& dataOut) const;
            bool deserialize(ByteBuffer& dataIn);

            stringImpl _textureName;
            stringImpl _texturePath;

            // Only Albedo/Diffuse should be sRGB
            // Normals, specular, etc should be in linear space
            bool _srgbSpace;
            TextureWrap _wrapU, _wrapV, _wrapW;
            Material::TextureOperation _operation;
        };

        struct MaterialData {
            MaterialData()
            {
                _ignoreAlpha = false;
                _doubleSided = true;
                _shadingMode = Material::ShadingMode::FLAT;
                _bumpMethod = Material::BumpMethod::NONE;
                _textures.fill(TextureEntry());
            }

            bool serialize(ByteBuffer& dataOut) const;
            bool deserialize(ByteBuffer& dataIn);

            bool _ignoreAlpha;
            bool _doubleSided;
            stringImpl _name;
            Material::ColourData   _colourData;
            Material::ShadingMode  _shadingMode;
            Material::BumpMethod   _bumpMethod;
            std::array<TextureEntry, to_base(ShaderProgram::TextureUsage::COUNT)> _textures;
        };

        struct SubMeshData {
            SubMeshData()
            {
                _boneCount = _index = 0;
                _partitionOffset = 0;
            }

            bool serialize(ByteBuffer& dataOut) const;
            bool deserialize(ByteBuffer& dataIn);

            stringImpl _name;
            U32 _index;
            U32 _boneCount;
            U32 _partitionOffset;
            vec3<F32> _minPos, _maxPos;
            vectorBest<vec3<U32>> _triangles;
            MaterialData _material;
        };

        struct ImportData {
            ImportData(const stringImpl& meshFile)
                : _modelPath(meshFile)
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
            bool _loadedFromFile;
            // Geometry
            VertexBuffer* _vertexBuffer;
            // Submeshes
            vector<SubMeshData> _subMeshData;
            // Animations
            Bone* _skeleton;
            vector<Bone*> _bones;
            vector<std::shared_ptr<AnimEvaluator>> _animations;
            bool _hasAnimations;

            // Name and path
            stringImpl _modelName;
            stringImpl _modelPath;
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