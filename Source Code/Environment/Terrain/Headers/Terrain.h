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

#include "TileRing.h"
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

enum class TerrainTextureChannel : U8 {
    TEXTURE_RED_CHANNEL = 0,
    TEXTURE_GREEN_CHANNEL = 1,
    TEXTURE_BLUE_CHANNEL = 2,
    TEXTURE_ALPHA_CHANNEL = 3,
    COUNT
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
     static constexpr I32 MAX_RINGS = 10;
     static constexpr I32 VTX_PER_TILE_EDGE = 9; // overlap => -2
     static constexpr I32 PATCHES_PER_TILE_EDGE = VTX_PER_TILE_EDGE - 1;
     static constexpr I32 QUAD_LIST_INDEX_COUNT = (VTX_PER_TILE_EDGE - 1) * (VTX_PER_TILE_EDGE - 1) * 4;
     static constexpr F32 WORLD_SCALE = 1.0f;

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

    const char* getResourceTypeName() const override { return "Terrain"; }

   public:
    vector<VertexBuffer::Vertex> _physicsVerts;

   protected:
    enum class EditorDataState : U8 {
        CHANGED = 0,
        QUEUED,
        IDLE
    };
    bool _drawBBoxes;
    U32  _nodeDataIndex = 0;
    VegetationDetails _vegDetails;

    Quadtree _terrainQuadtree;
    vector<TerrainChunk*> _terrainChunks;

    std::array<std::shared_ptr<TileRing>, MAX_RINGS>  _tileRings;
    EditorDataState _editorDataDirtyState;
    SceneGraphNode* _vegetationGrassNode;
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

    static U32 dataIdx(Terrain& terrain) noexcept {
        return terrain._nodeDataIndex;
    }

    friend class Divide::TerrainLoader;
};

};  // namespace Attorney
};  // namespace Divide

#endif
