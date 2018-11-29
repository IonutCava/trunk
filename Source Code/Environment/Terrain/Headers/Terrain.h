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

#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "TerrainTessellator.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Environment/Vegetation/Headers/Vegetation.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class TerrainLoader;

constexpr U8 MAX_TEXTURE_LAYERS = 128 / 4;

template <typename T>
inline T TER_COORD(T x, T y, T width) {
    return x + (width * y);
}

struct TerrainTextureLayer {
    explicit TerrainTextureLayer(U8 layerCount)
        : _layerCount(layerCount)
    {
        assert(_layerCount <= MAX_TEXTURE_LAYERS);

        _blendMaps = nullptr;
        _tileMaps = nullptr;
        _normalMaps = nullptr;
    }

    ~TerrainTextureLayer();

    enum class TerrainTextureChannel : U8 {
        TEXTURE_RED_CHANNEL = 0,
        TEXTURE_GREEN_CHANNEL = 1,
        TEXTURE_BLUE_CHANNEL = 2,
        TEXTURE_ALPHA_CHANNEL = 3
    };

    inline void setBlendMaps(const Texture_ptr& texture)  { _blendMaps = texture; }
    inline void setTileMaps(const Texture_ptr& texture, const vector<U8>& countPerLayer)   {
        _tileMaps = texture;
        _albedoCountPerLayer = countPerLayer;
    }

    inline void setNormalMaps(const Texture_ptr& texture, const vector<U8>& countPerLayer) {
        _normalMaps = texture;
        _detailCountPerLayer = countPerLayer;
    }

    inline void setDiffuseScale(TerrainTextureChannel textureChannel, U8 layer, F32 scale) {
        _diffuseUVScale[layer][to_U32(textureChannel)]= scale;
    }

    inline void setDetailScale(TerrainTextureChannel textureChannel, U8 layer, F32 scale) {
        _detailUVScale[layer][to_U32(textureChannel)] = scale;
    }

    inline const vec4<F32>& getDiffuseScales(U8 layer) const { return _diffuseUVScale[layer]; }
    inline const vec4<F32>& getDetailScales(U8 layer) const { return _detailUVScale[layer]; }

    inline const std::array<vec4<F32>, MAX_TEXTURE_LAYERS>& getDiffuseScales() const { return _diffuseUVScale; }
    inline const std::array<vec4<F32>, MAX_TEXTURE_LAYERS>& getDetailScales() const { return _detailUVScale; }

    const Texture_ptr& blendMaps()  const { return _blendMaps; }
    const Texture_ptr& tileMaps()   const { return _tileMaps; }
    const Texture_ptr& normalMaps() const { return _normalMaps; }

    inline U8 layerCount() const { return _layerCount; }
    inline U8 albedoCountPerLayer(U8 layer) const { assert(layer < _layerCount); return _albedoCountPerLayer[layer]; }
    inline U8 detailCountPerLayer(U8 layer) const { assert(layer < _layerCount); return _detailCountPerLayer[layer]; }

   private:
    U8 _layerCount;
    std::vector<U8> _albedoCountPerLayer;
    std::vector<U8> _detailCountPerLayer;

    std::array<vec4<F32>, MAX_TEXTURE_LAYERS> _diffuseUVScale;
    std::array<vec4<F32>, MAX_TEXTURE_LAYERS> _detailUVScale;
    Texture_ptr _blendMaps;
    Texture_ptr _tileMaps;
    Texture_ptr _normalMaps;
};

class VertexBuffer;
class TerrainDescriptor;

FWD_DECLARE_MANAGED_CLASS(Quad3D);
FWD_DECLARE_MANAGED_CLASS(Patch3D);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

namespace Attorney {
    class TerrainLoader;
};

class Terrain : public Object3D {
    friend class Attorney::TerrainLoader;
   public:
     static constexpr U32 MAX_RENDER_NODES = 384;
     static constexpr size_t NODE_DATA_SIZE = sizeof(TessellatedNodeData) * Terrain::MAX_RENDER_NODES * to_base(RenderStage::COUNT);
     static constexpr bool USE_TERRAIN_UBO = false;//NODE_DATA_SIZE < (64 * 1024);

   public:
    explicit Terrain(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name);
    ~Terrain();

    bool unload() override;

    inline void toggleBoundingBoxes() { _drawBBoxes = !_drawBBoxes; }

    vec3<F32> getPositionFromGlobal(F32 x, F32 z) const;
    vec3<F32> getPosition(F32 x_clampf, F32 z_clampf) const;
    vec3<F32> getNormal(F32 x_clampf, F32 z_clampf) const;
    vec3<F32> getTangent(F32 x_clampf, F32 z_clampf) const;
    vec2<U16> getDimensions() const;

   
    void saveToXML(boost::property_tree::ptree& pt) const override;
    void loadFromXML(const boost::property_tree::ptree& pt)  override;

   protected:
    void sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) override;

    void postBuild();

    void buildDrawCommands(SceneGraphNode& sgn,
                                RenderStagePass renderStagePass,
                                RenderPackage& pkgInOut) override;

    bool onRender(SceneGraphNode& sgn,
                  const SceneRenderState& sceneRenderState,
                  RenderStagePass renderStagePass) override;

    void postLoad(SceneGraphNode& sgn);

   public:
    hashMap<U32, vector<U32>> _physicsIndices;
    vector<VertexBuffer::Vertex> _physicsVerts;

   protected:
    ShaderBuffer* _shaderData;
    VegetationDetails _vegDetails;

    typedef std::array<bool, to_base(RenderStage::COUNT)> TessellatorArrayFlags;
    typedef std::array<TerrainTessellator, to_base(RenderStage::COUNT)> TessellatorArray;
    typedef hashMap<I64, bool> CameraUpdateFlagArray;


    TessellatorArray _terrainTessellator;
    hashMap<I64, TessellatorArrayFlags> _terrainTessellatorFlags;

    bool _drawBBoxes;
    Quad3D_ptr _plane;
    ShaderProgram_ptr _planeShader;
    ShaderProgram_ptr _planeDepthShader;
    SceneGraphNode* _vegetationGrassNode;
    TerrainTextureLayer* _terrainTextures;
    std::shared_ptr<TerrainDescriptor> _descriptor;
};

namespace Attorney {

class TerrainLoader {
   private:
    static void setTextureLayer(Terrain& terrain, TerrainTextureLayer* textureLayer) {
        terrain._terrainTextures = textureLayer;
    }

    static U32 textureLayerCount(Terrain& terrain) {
        return to_U32(terrain._terrainTextures->layerCount());
    }

    static VegetationDetails& vegetationDetails(Terrain& terrain) {
        return terrain._vegDetails;
    }

    static BoundingBox& boundingBox(Terrain& terrain) {
        return terrain._boundingBox;
    }

    static void postBuild(Terrain& terrain) {
        return terrain.postBuild();
    }

    static void plane(Terrain& terrain, Quad3D_ptr plane, const ShaderProgram_ptr& shader, const ShaderProgram_ptr& depthShader) {
        terrain._plane = plane;
        terrain._planeShader = shader;
        terrain._planeDepthShader = depthShader;
    }

    static void descriptor(Terrain& terrain, const std::shared_ptr<TerrainDescriptor>& descriptor) {
        terrain._descriptor = descriptor;
    }

    friend class Divide::TerrainLoader;
};

};  // namespace Attorney
};  // namespace Divide

#endif
