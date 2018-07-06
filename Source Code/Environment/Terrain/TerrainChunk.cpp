#include "Headers/TerrainChunk.h"
#include "Headers/Terrain.h"
#include "Quadtree/Headers/QuadtreeNode.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Vegetation/Headers/Vegetation.h"

namespace Divide {

U32 TerrainChunk::_chunkID = 0;

TerrainChunk::TerrainChunk(Terrain* const parentTerrain, QuadtreeNode* const parentNode) : _parentTerrain(parentTerrain),
                                                                                           _parentNode(parentNode),
                                                                                           _vegetation(nullptr)
{
    _chunkID++;
    _xOffset = _yOffset = _sizeX = _sizeY = 0;
    _chunkIndOffset = 0;
    memset(_lodIndOffset, 0, Config::TERRAIN_CHUNKS_LOD * sizeof(U32));
    memset(_lodIndCount, 0, Config::TERRAIN_CHUNKS_LOD * sizeof(U32));

    _terrainVB = _parentTerrain->getGeometryVB();

    VegetationDetails vegDetails = _parentTerrain->_vegDetails;
    vegDetails.name += stringAlg::toBase("_chunk_" + Util::toString(_chunkID));
    _vegetation = New Vegetation(vegDetails); //<Deleted by the sceneGraph on "unload"
    _vegetation->getSceneNodeRenderState().useDefaultMaterial(false);
    _vegetation->setMaterial(nullptr);
    assert(_vegetation != nullptr);
}

TerrainChunk::~TerrainChunk()
{
    for (U8 i = 0; i < Config::TERRAIN_CHUNKS_LOD; i++)
        _indice[i].clear();
    
    memset(_lodIndOffset, 0, Config::TERRAIN_CHUNKS_LOD * sizeof(U32));
    memset(_lodIndCount, 0, Config::TERRAIN_CHUNKS_LOD * sizeof(U32));
    _chunkIndOffset = 0;
    _terrainVB = nullptr;
}

void TerrainChunk::Load(U8 depth, const vec2<U32>& pos, U32 minHMSize, const vec2<U32>& HMsize, SceneGraphNode* const parentTerrainSGN){

    _chunkIndOffset = _terrainVB->getIndexCount();

    _xOffset = (F32)pos.x;
    _yOffset = (F32)pos.y;
    _sizeX = (F32)minHMSize;
    _sizeY = (F32)minHMSize;

    
    for(U8 i=0; i < Config::TERRAIN_CHUNKS_LOD; i++)
        ComputeIndicesArray(i, depth, pos, HMsize);

    const vectorImpl<vec3<F32>>& vertices = _terrainVB->getPosition();

    F32 tempMin = std::numeric_limits<F32>::max();
    F32 tempMax = std::numeric_limits<F32>::min();
    F32 height = 0.0f;
    for(U32 i = 0; i < _lodIndCount[0]; ++i){
        U32 idx = _indice[0][i];
        if (idx == Config::PRIMITIVE_RESTART_INDEX_L) continue;

        height = vertices[idx].y;

        if(height > tempMax) tempMax = height;
        if(height < tempMin) tempMin = height;
    }

    _heightBounds.set(tempMin, tempMax);

    for(U8 i = 0; i < Config::TERRAIN_CHUNKS_LOD; i++){
        _indice[i].clear();
    }
    parentTerrainSGN->addNode(_vegetation);
    _vegetation->initialize(this, parentTerrainSGN);
}

void TerrainChunk::ComputeIndicesArray(I8 lod, U8 depth, const vec2<U32>& position, const vec2<U32>& heightMapSize){
    assert(lod < Config::TERRAIN_CHUNKS_LOD);

    U32 offset = (U32)std::pow(2.0f, (F32)(lod));
    U32 div = (U32)std::pow(2.0f, (F32)(depth+lod));
    vec2<U32> heightmapDataSize = heightMapSize/(div);

    U32 nHMWidth   = heightmapDataSize.x + 1;
    U32 nHMHeight  = heightmapDataSize.y + 1;
    U32 nHMOffsetX = position.x;
    U32 nHMOffsetY = position.y;

    U32 nHMTotalWidth  = heightMapSize.x;
    U32 nIndice = (nHMWidth)*(nHMHeight-1)*2;
    nIndice += (nHMHeight-1);//<Restart indices
    _indice[lod].reserve( nIndice  );

    U32 iOffset = 0;
    U32 indexA = 0;
    U32 indexB = 0;
    U32 jOffset = 0;
    for(U16 j=0; j<nHMHeight-1; j++){
        jOffset = j*(offset) + nHMOffsetY;
        for(U16 i=0; i<nHMWidth; i++){
            iOffset = i*(offset) + nHMOffsetX;
            indexA = iOffset + jOffset * nHMTotalWidth;
            indexB = iOffset + (jOffset + (offset)) * nHMTotalWidth;
            _indice[lod].push_back(indexA);
            _indice[lod].push_back(indexB);
            _terrainVB->addIndexL(indexA);
            _terrainVB->addIndexL(indexB);
        }
        _indice[lod].push_back(Config::PRIMITIVE_RESTART_INDEX_L);
        _terrainVB->addRestartIndex();
    }

    if(lod > 0) _lodIndOffset[lod] = (U32)(_indice[lod-1].size() + _lodIndOffset[lod-1]);

    _lodIndCount[lod] = (U32)_indice[lod].size();
    assert(nIndice == _lodIndCount[lod]);
}

void TerrainChunk::CreateDrawCommand(I8 lod) {
    assert(lod < Config::TERRAIN_CHUNKS_LOD);
    if(lod > 0) lod--;
        
    GenericDrawCommand drawCommand(TRIANGLE_STRIP, _lodIndOffset[lod] + _chunkIndOffset, _lodIndCount[lod]);
    drawCommand.LoD(lod);
    _parentTerrain->addDrawCommand(drawCommand);
}

U8 TerrainChunk::getLoD() const { 
    return _parentNode->getLoD();
 }

};