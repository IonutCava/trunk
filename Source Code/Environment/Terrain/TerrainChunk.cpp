#include "Headers/TerrainChunk.h"
#include "Headers/Terrain.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"

void TerrainChunk::Load(U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize){
    for(U8 i=0; i < Config::TERRAIN_CHUNKS_LOD; i++) 
        ComputeIndicesArray(i, depth, pos, HMsize);

    _grassData._grassVisibility = GET_ACTIVE_SCENE()->state().getGrassVisibility();
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

    //U32 nHMTotalHeight = heightMapSize.y;
    U32 nHMTotalWidth  = heightMapSize.x;
    U32 nIndice = (nHMWidth)*(nHMHeight-1)*2;
    nIndice += (nHMHeight-1);//<Restart indices
    _indice[lod].reserve( nIndice  );
    _indOffsetW[lod] = nHMWidth*2;
    _indOffsetH[lod] = nHMWidth-1;

    for(U16 j=0; j<nHMHeight-1; j++){
        U32 jOffset = j*(offset) + nHMOffsetY;

        for(U16 i=0; i<nHMWidth; i++){
            U32 iOffset = i*(offset) + nHMOffsetX;

            _indice[lod].push_back(iOffset + jOffset * nHMTotalWidth);
            _indice[lod].push_back(iOffset + (jOffset + (offset)) * nHMTotalWidth);
        }
        _indice[lod].push_back(TERRAIN_STRIP_RESTART_INDEX);
    }

    assert(nIndice == _indice[lod].size());
}

void TerrainChunk::Destroy(){
    for(U8 i = 0; i < Config::TERRAIN_CHUNKS_LOD; i++){
        _indice[i].clear();
        _grassData._grassVisibility = _indOffsetW[i] = _indOffsetH[i] = 0;
    }
}

I32 TerrainChunk::DrawGround(I8 lod, ShaderProgram* const program, VertexBufferObject* const vbo){
    assert(lod < Config::TERRAIN_CHUNKS_LOD);
    if(lod>0) lod--;

    program->Uniform("LODFactor", 1.0f / (lod+1));
    
    vbo->setFirstElement(&(_indice[lod][0]));
    vbo->setRangeCount(_indice[lod].size());
    GFX_DEVICE.renderBuffer(vbo);

    return 1;
}

#pragma message("Grass rendering disabled for now! -Ionut")
void  TerrainChunk::DrawGrass(I8 lod, F32 d,U32 geometryIndex, Transform* const parentTransform){
    assert(lod < Config::TERRAIN_CHUNKS_LOD);
    if(lod != 0) return;
    if(d > _grassData._grassVisibility) { //if we go beyond grass visibility limit ...
        return; // ... do not draw grass
    }

    if(!_grassData._grassIndice[geometryIndex].empty())	{
        F32 ratio = std::min((1.0f - (d / _grassData._grassVisibility)) * 2.0f, 1.0f);

        U32 indices_count = (U32)( ratio * _grassData._grassIndice[geometryIndex].size() );

        if(indices_count > 0 && false){
            _grassData._grassVBO->setFirstElement(&(_grassData._grassIndice[geometryIndex].front()));
            _grassData._grassVBO->setRangeCount(indices_count);
            GFX_DEVICE.renderBuffer(_grassData._grassVBO, parentTransform);
        }
    }
}

typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;
void TerrainChunk::addTree(const vec4<F32>& pos,F32 scale, const FileData& tree, SceneGraphNode* parentNode){
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
        for_each(SceneGraphNode::NodeChildren::value_type& it, treeNode->getChildren()){
            assert(it.second);
            Material* m = (it.second)->getNode<SceneNode>()->getMaterial();
            if(m){
                m->addShaderDefines("ADD_FOLIAGE, IS_TREE");
                m->addShaderModifier("Tree");///<Just to create a different shader in the ResourceCahe
            }
        }
        if(tree.staticUsage){
            treeNode->setUsageContext(SceneGraphNode::NODE_STATIC);
        }
        if(tree.navigationUsage){
            treeNode->setNavigationContext(SceneGraphNode::NODE_OBSTACLE);
        }
        if(tree.physicsUsage){
            treeNode->setPhysicsGroup(SceneGraphNode::NODE_COLLIDE_NO_PUSH);
        }
    }else{
        ERROR_FN(Locale::get("ERROR_ADD_TREE"),tree.ModelName.c_str());
    }
}