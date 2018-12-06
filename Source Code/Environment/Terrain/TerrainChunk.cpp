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
      _terrain(parentTerrain),
      _vegetation(nullptr),
      _lodIndCount(0),
      _chunkIndOffset(0),
      _lodIndOffset(0),
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
    vector<U32>& indices = _terrain->_physicsIndices[_ID];
    _chunkIndOffset = to_U32(indices.size());

    _xOffset = to_F32(pos.x);
    _yOffset = to_F32(pos.y);
    _sizeX = _sizeY = to_F32(_targetChunkDimension);

    computeIndicesArray(depth, pos, HMsize);

    F32 tempMin = std::numeric_limits<F32>::max();
    F32 tempMax = std::numeric_limits<F32>::min();

    for (U32 i = 0; i < _lodIndCount; ++i) {
        U32 idx = indices[i];
        if (idx == Config::PRIMITIVE_RESTART_INDEX_L) {
            continue;
        }

        F32 height = _terrain->_physicsVerts[idx]._position.y;

        if (height > tempMax) {
            tempMax = height;
        }
        if (height < tempMin) {
            tempMin = height;
        }
    }

    _heightBounds.set(tempMin, tempMax);
    _vegetation = std::make_shared<Vegetation>(_context, *this, Attorney::TerrainChunk::vegetationDetails(parent()));
    Attorney::TerrainChunk::registerTerrainChunk(*_parentTerrain, this);
}

void TerrainChunk::computeIndicesArray(U8 depth,
                                       const vec2<U32>& position,
                                       const vec2<U32>& heightMapSize) {
    U32 offset = to_U32(std::pow(2.0f, to_F32(0.0f)));
    U32 div = to_U32(std::pow(2.0f, to_F32(depth)));

    vec2<U32> heightmapDataSize = heightMapSize / (div);

    U32 nHMWidth = heightmapDataSize.x;
    U32 nHMHeight = heightmapDataSize.y;
    U32 nHMOffsetX = position.x;
    U32 nHMOffsetY = position.y;

    vector<U32>& indices = _terrain->_physicsIndices[_ID];

    U32 nHMTotalWidth = heightMapSize.x;
    U32 nIndice = (nHMWidth) * (nHMHeight - 1) * 2;
    nIndice += (nHMHeight - 1);  //<Restart indices
    indices.reserve(nIndice);

    U32 iOffset = 0;
    U32 jOffset = 0;
    for (U16 j = 0; j < nHMHeight - 1; j++) {
        jOffset = j * (offset) + nHMOffsetY;
        for (U16 i = 0; i < nHMWidth; i++) {
            iOffset = i * (offset) + nHMOffsetX;
            indices.push_back(iOffset + jOffset * nHMTotalWidth);
            indices.push_back(iOffset + (jOffset + (offset)) * nHMTotalWidth);
        }
        indices.push_back(Config::PRIMITIVE_RESTART_INDEX_L);
    }

    _lodIndCount = to_U32(indices.size());
    assert(nIndice == _lodIndCount);
}

const BoundingBox& TerrainChunk::bounds() const {
    return _quadtreeNode.getBoundingBox();
}

};