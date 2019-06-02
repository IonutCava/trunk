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
#include "Environment/Terrain/Quadtree/Headers/Quadtree.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class TerrainLoader;

template <typename T>
inline T TER_COORD(T x, T y, T width) {
    return x + (width * y);
}

struct TerrainTextureLayer {
    explicit TerrainTextureLayer(U8 layerCount)
    {
        _blendMaps = nullptr;
        _tileMaps = nullptr;
        _normalMaps = nullptr;

        _albedoCountPerLayer.resize(layerCount);
        _detailCountPerLayer.resize(layerCount);
    }

    ~TerrainTextureLayer();

    enum class TerrainTextureChannel : U8 {
        TEXTURE_RED_CHANNEL = 0,
        TEXTURE_GREEN_CHANNEL = 1,
        TEXTURE_BLUE_CHANNEL = 2,
        TEXTURE_ALPHA_CHANNEL = 3
    };

    inline void setBlendMaps(const Texture_ptr& texture) noexcept { _blendMaps = texture; }
    inline void setTileMaps(const Texture_ptr& texture, const vector<U8>& countPerLayer)   {
        _tileMaps = texture;
        _albedoCountPerLayer = countPerLayer;
    }

    inline void setNormalMaps(const Texture_ptr& texture, const vector<U8>& countPerLayer) {
        _normalMaps = texture;
        _detailCountPerLayer = countPerLayer;
    }

    const Texture_ptr& blendMaps()  const noexcept { return _blendMaps; }
    const Texture_ptr& tileMaps()   const noexcept { return _tileMaps; }
    const Texture_ptr& normalMaps() const noexcept { return _normalMaps; }

    inline U8 layerCount() const noexcept { return to_U8(_albedoCountPerLayer.size()); }
    inline U8 albedoCountPerLayer(U8 layer) const { assert(layer < layerCount()); return _albedoCountPerLayer[layer]; }
    inline U8 detailCountPerLayer(U8 layer) const { assert(layer < layerCount()); return _detailCountPerLayer[layer]; }

   private:
    vector<U8> _albedoCountPerLayer;
    vector<U8> _detailCountPerLayer;
    Texture_ptr _blendMaps;
    Texture_ptr _tileMaps;
    Texture_ptr _normalMaps;
};

class VertexBuffer;
class TerrainChunk;
class TerrainDescriptor;

FWD_DECLARE_MANAGED_CLASS(Quad3D);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

namespace Attorney {
    class TerrainChunk;
    class TerrainLoader;
};

class Terrain : public Object3D {
    friend class Attorney::TerrainChunk;
    friend class Attorney::TerrainLoader;

   public:
     static constexpr U32 MAX_RENDER_NODES = 1024;
     static constexpr size_t NODE_DATA_SIZE = sizeof(TessellatedNodeData) * Terrain::MAX_RENDER_NODES * to_base(RenderStage::COUNT);

   public:
       struct Vert {
           vec3<F32> _position;
           vec3<F32> _normal;
           vec3<F32> _tangent;
       };

   public:
    explicit Terrain(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name);
    virtual ~Terrain();

    bool unload() noexcept override;

    inline void toggleBoundingBoxes() noexcept { _drawBBoxes = !_drawBBoxes; }

    Vert getVert(F32 x_clampf, F32 z_clampf, bool smooth) const;

    vec3<F32> getPositionFromGlobal(F32 x, F32 z, bool smooth) const;
    vec3<F32> getPosition(F32 x_clampf, F32 z_clampf, bool smooth) const;
    vec3<F32> getNormal(F32 x_clampf, F32 z_clampf, bool smooth) const;
    vec3<F32> getTangent(F32 x_clampf, F32 z_clampf, bool smooth) const;
    vec2<U16> getDimensions() const;
    vec2<F32> getAltitudeRange() const;

    inline const Quadtree& getQuadtree() const noexcept { return _terrainQuadtree; }

    void saveToXML(boost::property_tree::ptree& pt) const override;
    void loadFromXML(const boost::property_tree::ptree& pt)  override;

   protected:
    Vert getVert(F32 x_clampf, F32 z_clampf) const;
    Vert getSmoothVert(F32 x_clampf, F32 z_clampf) const;

    void frameStarted(SceneGraphNode& sgn) override;

    void sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) override;

    void postBuild();

    void buildDrawCommands(SceneGraphNode& sgn,
                           RenderStagePass renderStagePass,
                           RenderPackage& pkgInOut) override;

    bool onRender(SceneGraphNode& sgn,
                  const Camera& camera,
                  RenderStagePass renderStagePass,
                  bool refreshData) override;

    void postLoad(SceneGraphNode& sgn);

    void onEditorChange(EditorComponentField& field);


   public:
    vector<VertexBuffer::Vertex> _physicsVerts;

   protected:
    enum class EditorDataState : U8 {
        CHANGED = 0,
        QUEUED,
        IDLE
    };

    F32 _drawDistance;
    I32 _initBufferWriteCounter = 0;
    ShaderBuffer* _shaderData;
    VegetationDetails _vegDetails;

    typedef std::array<TerrainTessellator, to_base(RenderStage::COUNT)-1> TessellatorArray;
    typedef hashMap<I64, bool> CameraUpdateFlagArray;

    Quadtree _terrainQuadtree;
    vector<TerrainChunk*> _terrainChunks;

    TessellatorArray _terrainTessellator;
    std::array<TerrainTessellator, ShadowMap::MAX_SHADOW_PASSES> _shadowTessellators;

    EditorDataState _editorDataDirtyState;
    bool _drawBBoxes;
    bool _shaderDataDirty;
    SceneGraphNode* _vegetationGrassNode;
    TerrainTextureLayer* _terrainTextures;
    std::shared_ptr<TerrainDescriptor> _descriptor;
};

namespace Attorney {
class TerrainChunk {
private:
    static const VegetationDetails& vegetationDetails(const Terrain& terrain) noexcept {
        return terrain._vegDetails;
    }

    static void registerTerrainChunk(Terrain& terrain, Divide::TerrainChunk* const chunk) {
        terrain._terrainChunks.push_back(chunk);
    }

    friend class Divide::TerrainChunk;
};

class TerrainLoader {
   private:
    static void setTextureLayer(Terrain& terrain, TerrainTextureLayer* textureLayer) noexcept {
        terrain._terrainTextures = textureLayer;
    }

    static U32 textureLayerCount(Terrain& terrain) noexcept {
        return to_U32(terrain._terrainTextures->layerCount());
    }

    static VegetationDetails& vegetationDetails(Terrain& terrain) noexcept {
        return terrain._vegDetails;
    }

    static BoundingBox& boundingBox(Terrain& terrain) noexcept {
        return terrain._boundingBox;
    }

    static void postBuild(Terrain& terrain) {
        return terrain.postBuild();
    }

    static void descriptor(Terrain& terrain, const std::shared_ptr<TerrainDescriptor>& descriptor) noexcept {
        terrain._descriptor = descriptor;
    }

    friend class Divide::TerrainLoader;
};

};  // namespace Attorney
};  // namespace Divide

#endif
