#include "Headers/TerrainChunk.h"
#include "Headers/Terrain.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"

void TerrainChunk::Load(U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize, VertexBufferObject* const vbo){
    memset(_lodIndOffset, 0, Config::TERRAIN_CHUNKS_LOD * sizeof(U32));
    memset(_lodIndCount, 0, Config::TERRAIN_CHUNKS_LOD * sizeof(U32));

    _chunkIndOffset = vbo->getIndexCount();

    for(U8 i=0; i < Config::TERRAIN_CHUNKS_LOD; i++)
        ComputeIndicesArray(i, depth, pos, HMsize, vbo);

    const vectorImpl<vec3<F32>>& vertices = vbo->getPosition();

    F32 tempMin = 100000.0f;
    F32 tempMax = -100000.0f;
    for(U32 i = 0; i < _lodIndCount[0]; ++i){
        if(_indice[0][i] == TERRAIN_STRIP_RESTART_INDEX) continue;

        F32 height = vertices[_indice[0][i]].y;

        if(height > tempMax) tempMax = height;
        if(height < tempMin) tempMin = height;
    }

    _heightBounds[0] = tempMin;
    _heightBounds[1] = tempMax;

    for(U8 i = 0; i < Config::TERRAIN_CHUNKS_LOD; i++){
        _indice[i].clear();
    }
}

void TerrainChunk::ComputeIndicesArray(I8 lod, U8 depth, const vec2<U32>& position, const vec2<U32>& heightMapSize, VertexBufferObject* const vbo){
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
    _indOffsetW[lod] = nHMWidth*2;
    _indOffsetH[lod] = nHMWidth-1;

    for(U16 j=0; j<nHMHeight-1; j++){
        U32 jOffset = j*(offset) + nHMOffsetY;

        for(U16 i=0; i<nHMWidth; i++){
            U32 iOffset = i*(offset) + nHMOffsetX;
            U32 indexA = iOffset + jOffset * nHMTotalWidth;
            U32 indexB = iOffset + (jOffset + (offset)) * nHMTotalWidth;
            _indice[lod].push_back(indexA);
            _indice[lod].push_back(indexB);
            vbo->addIndex(indexA);
            vbo->addIndex(indexB);
        }
        _indice[lod].push_back(TERRAIN_STRIP_RESTART_INDEX);
        vbo->addIndex(TERRAIN_STRIP_RESTART_INDEX);
    }

    if(lod > 0) _lodIndOffset[lod] = (U32)(_indice[lod-1].size() + _lodIndOffset[lod-1]);

    _lodIndCount[lod] = (U32)_indice[lod].size();
    assert(nIndice == _lodIndCount[lod]);
}

void TerrainChunk::GenerateGrassIndexBuffer(U32 bilboardCount) {
	if(_grassData._grassVBO == nullptr)
		return;

    // the first offset is always 0;
    _grassData._grassIndexOffset[0] = 0;
    // get the index vector for every billboard texture
    for(U8 index = 0; index < bilboardCount; ++index){
        const vectorImpl<U32>& grassIndices = _grassData._grassIndices[index];
        // add every index for the current texture to the VBO
        for(U32 i = 0; i < grassIndices.size(); ++i){
            _grassData._grassVBO->addIndex(grassIndices[i]);
        }
        // save the number of indices for the current texture
        _grassData._grassIndexSize[index] = (U32)grassIndices.size();
        // clear the memory used by the original indices
        _grassData._grassIndices[index].clear();
        // calculate the needed index offset for the current billboard texture
        if(index > 0) _grassData._grassIndexOffset[index] = _grassData._grassIndexSize[index - 1] + _grassData._grassIndexOffset[index - 1];
    }

    _grassData._grassVisibility = GET_ACTIVE_SCENE()->state().getGrassVisibility();
}

void TerrainChunk::Destroy(){
    for(U8 i = 0; i < Config::TERRAIN_CHUNKS_LOD; i++){
        _indOffsetW[i] = _indOffsetH[i] = 0;
    }

    _grassData._grassVisibility = 0;
}

I32 TerrainChunk::DrawGround(I8 lod, ShaderProgram* const program, VertexBufferObject* const vbo){
    assert(lod < Config::TERRAIN_CHUNKS_LOD);
    if(lod>0) lod--;
   
    vbo->setFirstElement(_lodIndOffset[lod] + _chunkIndOffset);
    vbo->setRangeCount(_lodIndCount[lod]);
    GFX_DEVICE.renderBuffer(vbo);

    return 1;
}

void  TerrainChunk::DrawGrass(I8 lod, F32 distance, U32 index, Transform* const parentTransform){
    assert(lod < Config::TERRAIN_CHUNKS_LOD);
    if(distance > _grassData._grassVisibility) { //if we go beyond grass visibility limit ...
        return; // ... do not draw grass if the terrain chunk parent is too far away.
    }

    U32 indicesCount = _grassData._grassIndexSize[index];

    if(indicesCount > 0){
		assert(_grassData._grassVBO != nullptr);
		_grassData._grassVBO->currentShader()->Uniform("grassVisibilityRange", _grassData._grassVisibility);
        _grassData._grassVBO->setFirstElement(_grassData._grassIndexOffset[index]);
        _grassData._grassVBO->setRangeCount(indicesCount);
        GFX_DEVICE.renderBuffer(_grassData._grassVBO, parentTransform);
    }
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
            Material* m = (it.second)->getSceneNode()->getMaterial();
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