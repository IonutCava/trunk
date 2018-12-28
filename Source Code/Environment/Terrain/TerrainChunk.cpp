#include "stdafx.h"

#include "Headers/TerrainChunk.h"
#include "Headers/Terrain.h"
#include "Quadtree/Headers/QuadtreeNode.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Vegetation/Headers/Vegetation.h"

namespace Divide {

U32 TerrainChunk::_chunkID = 0;

TerrainChunk::TerrainChunk(GFXDevice& context,
                           Terrain* const parentTerrain,
                           const QuadtreeNode& parentNode)
    : _context(context),
      _quadtreeNode(parentNode),
      _parentTerrain(parentTerrain),
      _ID(_chunkID++),
      _vegetation(nullptr),
      _xOffset(0),
      _yOffset(0),
      _sizeX(0),
      _sizeY(0)
{
}

TerrainChunk::~TerrainChunk()
{
}

void TerrainChunk::load(U8 depth, const vec2<U32>& pos, U32 _targetChunkDimension, const vec2<U32>& HMsize) {
    _xOffset = to_F32(pos.x);
    _yOffset = to_F32(pos.y);
    _sizeX = _sizeY = to_F32(_targetChunkDimension);

    F32 tempMin = std::numeric_limits<F32>::max();
    F32 tempMax = std::numeric_limits<F32>::min();

    U32 offset = to_U32(std::pow(2.0f, to_F32(0.0f)));
    U32 div = to_U32(std::pow(2.0f, to_F32(depth)));

    vec2<U32> heightmapDataSize = HMsize / (div);
    U32 nHMWidth = heightmapDataSize.x;
    U32 nHMHeight = heightmapDataSize.y;

    const vector<VertexBuffer::Vertex>& verts = _parentTerrain->_physicsVerts;

    for (U16 j = 0; j < nHMHeight - 1; ++j) {
        U32 jOffset = j * (offset)+pos.y;
        for (U16 i = 0; i < nHMWidth; ++i) {
            U32 iOffset = i * (offset)+pos.x;
            U32 idx = iOffset + jOffset * HMsize.x;
            F32 height = verts[idx]._position.y;

            if (height > tempMax) {
                tempMax = height;
            }
            if (height < tempMin) {
                tempMin = height;
            }


            idx = iOffset + (jOffset + (offset)) * HMsize.x;
            height = verts[idx]._position.y;

            if (height > tempMax) {
                tempMax = height;
            }
            if (height < tempMin) {
                tempMin = height;
            }
        }
    }

    _heightBounds.set(tempMin, tempMax);
    _vegetation = std::make_shared<Vegetation>(_context, *this, Attorney::TerrainChunk::vegetationDetails(parent()));
    Attorney::TerrainChunk::registerTerrainChunk(*_parentTerrain, this);
}


const BoundingBox& TerrainChunk::bounds() const {
    return _quadtreeNode.getBoundingBox();
}

bool TerrainChunk::isInView() const {
    return _quadtreeNode.isVisible();
}

};