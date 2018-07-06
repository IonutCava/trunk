#include "Headers/TerrainChunk.h"
#include "Headers/Terrain.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"

TerrainChunk::TerrainChunk(VertexBuffer* const groundVB, Terrain* const parentTerrain) : _terrainVB(groundVB), _parentTerrain(parentTerrain)
{
    _chunkIndOffset = 0;
    memset(_lodIndOffset, 0, Config::TERRAIN_CHUNKS_LOD * sizeof(U32));
    memset(_lodIndCount, 0, Config::TERRAIN_CHUNKS_LOD * sizeof(U32));
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

void TerrainChunk::Load(U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize){

    _chunkIndOffset = _terrainVB->getIndexCount();

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
}

void TerrainChunk::ComputeIndicesArray(I8 lod, U8 depth, const vec2<U32>& position, const vec2<U32>& heightMapSize){
    assert(lod < Config::TERRAIN_CHUNKS_LOD);

    U32 offset = (U32)pow(2.0f, (F32)(lod));
    U32 div = (U32)pow(2.0f, (F32)(depth+lod));
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

void TerrainChunk::DrawGround(I8 lod) const {
    assert(lod < Config::TERRAIN_CHUNKS_LOD);
    if(lod > 0) lod--;

    _terrainVB->setFirstElement(_lodIndOffset[lod] + _chunkIndOffset);
    _terrainVB->setRangeCount(_lodIndCount[lod]);
    _parentTerrain->renderChunkCallback(lod);
}

void TerrainChunk::addTree(const vec4<F32>& pos,F32 scale, const FileData& tree, SceneGraphNode* parentNode){
    typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;

    ResourceDescriptor model(tree.ItemName);
    model.setResourceLocation(tree.ModelName);
    model.setFlag(false);
    Mesh* tempTree = CreateResource<Mesh>(model);
    if(tempTree){
        std::stringstream ss; ss << "_" << tempTree->getRefCount();
        std::string treeName(tempTree->getName()+ss.str());
        ss.clear();
        SceneGraphNode* treeNode = parentNode->addNode(tempTree,treeName);
        PRINT_FN(Locale::get("TREE_ADDED"),treeNode->getName().c_str());
        Transform* treeTransform = treeNode->getTransform();
        treeTransform->scale(scale * tree.scale);
        treeTransform->rotateY(pos.w);
        treeTransform->translate(vec3<F32>(pos));
        FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, treeNode->getChildren()){
            assert(it.second);
            Material* m = (it.second)->getNode()->getMaterial();
            if(m){
                m->addShaderDefines("ADD_FOLIAGE, IS_TREE");
                m->addShaderModifier("Tree");///<Just to create a different shader in the ResourceCahe
            }
        }
        if(tree.staticUsage){
            treeNode->setUsageContext(SceneGraphNode::NODE_STATIC);
        }
        if(tree.navigationUsage){
            treeNode->getComponent<NavigationComponent>()->setNavigationContext(NavigationComponent::NODE_OBSTACLE);
        }
        if(tree.physicsUsage){
            treeNode->getComponent<PhysicsComponent>()->setPhysicsGroup(PhysicsComponent::NODE_COLLIDE_NO_PUSH);
        }
    }else{
        ERROR_FN(Locale::get("ERROR_ADD_TREE"),tree.ModelName.c_str());
    }
}