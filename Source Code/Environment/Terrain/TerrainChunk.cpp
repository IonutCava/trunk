#include "Headers/TerrainChunk.h"
#include "Headers/Terrain.h"
#include "Quadtree/Headers/QuadtreeNode.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Vegetation/Headers/Vegetation.h"

namespace Divide {

U32 TerrainChunk::_chunkID = 0;

TerrainChunk::TerrainChunk(GFXDevice& context,
                           Terrain* const parentTerrain,
                           QuadtreeNode* const parentNode)
    : _parentNode(parentNode), 
      _parentTerrain(parentTerrain),
      _vegetation(nullptr)
{
    _chunkID++;
    _xOffset = _yOffset = _sizeX = _sizeY = 0;
    _chunkIndOffset = 0;
    _lodIndOffset.fill(0);
    _lodIndCount.fill(0);

    _terrainVB = parentTerrain->getGeometryVB();

    VegetationDetails& vegDetails =  Attorney::TerrainChunk::vegetationDetails(*parentTerrain);
    vegDetails.name += "_chunk_" + to_stringImpl(_chunkID);
    //<Deleted by the sceneGraph on "unload"
    _vegetation = MemoryManager_NEW Vegetation(context, parentTerrain->parentResourceCache(), parentTerrain->getDescriptorHash() + _chunkID, vegDetails);
    _vegetation->renderState().useDefaultMaterial(false);
    _vegetation->setMaterialTpl(nullptr);
    assert(_vegetation != nullptr);
}

TerrainChunk::~TerrainChunk()
{
    for (U8 i = 0; i < Config::TERRAIN_CHUNKS_LOD; i++) {
        _indice[i].clear();
    }

    _lodIndOffset.fill(0);
    _lodIndCount.fill(0);
    _chunkIndOffset = 0;
    _terrainVB = nullptr;
    MemoryManager::DELETE(_vegetation);
}

void TerrainChunk::load(U8 depth, const vec2<U32>& pos, U32 _targetChunkDimension, const vec2<U32>& HMsize) {
    _chunkIndOffset = _terrainVB->getIndexCount();

    _xOffset = to_F32(pos.x);
    _yOffset = to_F32(pos.y);
    _sizeX = _sizeY = to_F32(_targetChunkDimension);

    for (U8 i = 0; i < Config::TERRAIN_CHUNKS_LOD; i++) {
        ComputeIndicesArray(i, depth, pos, HMsize);
        _terrainVB->addIndices(_indice[i], true);
    }

    F32 height = 0.0f;
    F32 tempMin = std::numeric_limits<F32>::max();
    F32 tempMax = std::numeric_limits<F32>::min();
    for (U32 i = 0; i < _lodIndCount[0]; ++i) {
        U32 idx = _indice[0][i];
        if (idx == Config::PRIMITIVE_RESTART_INDEX_L) {
            continue;
        }
        height = _terrainVB->getPosition(idx).y;

        if (height > tempMax) {
            tempMax = height;
        }
        if (height < tempMin) {
            tempMin = height;
        }
    }

    _heightBounds.set(tempMin, tempMax);

    for (U8 i = 0; i < Config::TERRAIN_CHUNKS_LOD; i++) {
        _indice[i].clear();
    }

    _vegetation->initialize(this);

    Attorney::TerrainChunk::registerTerrainChunk(*_parentTerrain, this);
}

void TerrainChunk::ComputeIndicesArray(I8 lod,
                                       U8 depth,
                                       const vec2<U32>& position,
                                       const vec2<U32>& heightMapSize) {
    assert(lod < Config::TERRAIN_CHUNKS_LOD);

    U32 offset = to_U32(std::pow(2.0f, to_F32(lod)));
    U32 div = to_U32(std::pow(2.0f, to_F32(depth + lod)));
    vec2<U32> heightmapDataSize = heightMapSize / (div);

    U32 nHMWidth = heightmapDataSize.x + 1;
    U32 nHMHeight = heightmapDataSize.y + 1;
    U32 nHMOffsetX = position.x;
    U32 nHMOffsetY = position.y;

    U32 nHMTotalWidth = heightMapSize.x;
    U32 nIndice = (nHMWidth) * (nHMHeight - 1) * 2;
    nIndice += (nHMHeight - 1);  //<Restart indices
    _indice[lod].reserve(nIndice);

    U32 iOffset = 0;
    U32 jOffset = 0;
    for (U16 j = 0; j < nHMHeight - 1; j++) {
        jOffset = j * (offset) + nHMOffsetY;
        for (U16 i = 0; i < nHMWidth; i++) {
            iOffset = i * (offset) + nHMOffsetX;
            _indice[lod].push_back(iOffset + jOffset * nHMTotalWidth);
            _indice[lod].push_back(iOffset + (jOffset + (offset)) * nHMTotalWidth);
        }
        _indice[lod].push_back(Config::PRIMITIVE_RESTART_INDEX_L);
    }

    if (lod > 0) {
        _lodIndOffset[lod] =
            to_U32(_indice[lod - 1].size() + _lodIndOffset[lod - 1]);
    }

    _lodIndCount[lod] = to_U32(_indice[lod].size());
    assert(nIndice == _lodIndCount[lod]);
}

vec3<U32> TerrainChunk::getBufferOffsetAndSize(I8 targetLoD) const {
    assert(targetLoD < Config::TERRAIN_CHUNKS_LOD);
    if (targetLoD > 0) {
        targetLoD--;
    }
    return vec3<U32>(_lodIndOffset[targetLoD] + _chunkIndOffset, _lodIndCount[targetLoD], to_U32(targetLoD));

}


U8 TerrainChunk::getLoD(const SceneRenderState& sceneRenderState) const {
    return _parentNode->getLoD(sceneRenderState);
}

F32 TerrainChunk::waterHeight() const {
    return Attorney::TerrainChunk::waterHeight(*_parentTerrain);
}

};