#include "Headers/TerrainChunk.h"
#include "Headers/Terrain.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"

typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;
void TerrainChunk::Load(U8 depth, vec2<U32> pos, vec2<U32> HMsize,VertexBufferObject* const groundVBO){
	for(U8 i=0; i < TERRAIN_CHUNKS_LOD; i++) ComputeIndicesArray(i, depth, pos, HMsize);
    for(U32 i=0; i < _indice[0].size(); i++) {
		if(_indice[0][i] == TERRAIN_STRIP_RESTART_INDEX) continue;
		groundVBO->addIndex(_indice[0][i]);
	}

	_grassData._grassVisibility = GET_ACTIVE_SCENE()->state()->getGrassVisibility();
}

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
	}else{
		ERROR_FN(Locale::get("ERROR_ADD_TREE"),tree.ModelName.c_str());
	}
}

void TerrainChunk::ComputeIndicesArray(I8 lod, U8 depth, const vec2<U32>& position, const vec2<U32>& heightMapSize){
	assert(lod < TERRAIN_CHUNKS_LOD);

	vec2<U32> vHeightmapDataPos = position;
	U32 offset = (U32)pow(2.0f, (F32)(lod));
	U32 div = (U32)pow(2.0f, (F32)(depth+lod));
	vec2<U32> vHeightmapDataSize = heightMapSize/(div) + vec2<U32>(1,1);

	U32 nHMWidth   = vHeightmapDataSize.x;
	U32 nHMHeight  = vHeightmapDataSize.y;
	U32 nHMOffsetX = vHeightmapDataPos.x;
	U32 nHMOffsetY = vHeightmapDataPos.y;

	U32 nHMTotalWidth  = heightMapSize.x;
	//U32 nHMTotalHeight = heightMapSize.y;

	_indOffsetW[lod] = nHMWidth*2;
	_indOffsetH[lod] = nHMWidth-1;
	U32 nIndice = (nHMWidth)*(nHMHeight-1)*2;
	nIndice += (nHMHeight-1);//<Restart indices
	_indice[lod].reserve( nIndice  );

	for(U16 j=0; j<nHMHeight-1; j++){
		for(U16 i=0; i<nHMWidth; i++){
			U32 id0 = (j*(offset) + nHMOffsetY+0)*(nHMTotalWidth)+(i*(offset) + nHMOffsetX+0);
			U32 id1 = (j*(offset) + nHMOffsetY+(offset))*(nHMTotalWidth)+(i*(offset) + nHMOffsetX+0);
			_indice[lod].push_back( id0 );
			_indice[lod].push_back( id1 );
		}
		_indice[lod].push_back(TERRAIN_STRIP_RESTART_INDEX);
	}

	assert(nIndice == _indice[lod].size());
}

void TerrainChunk::Destroy(){
	for(U8 i=0; i<TERRAIN_CHUNKS_LOD; i++){
		_indice[i].clear();
		_indOffsetW[i] = 0;
		_indOffsetH[i] = 0;
		_grassData._grassVisibility = 0;
	}
}

I32 TerrainChunk::DrawGround(I8 lod, ShaderProgram* const program, VertexBufferObject* const vbo){
	assert(lod < TERRAIN_CHUNKS_LOD);

	if(lod>0) lod--;

    if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE | REFLECTION_STAGE)){
        program->Uniform("LODFactor", 1.0f / (lod+1));
        assert(program->isBound());
    }

	vbo->setFirstElement(&(_indice[lod][0]));
	vbo->setRangeCount(_indOffsetW[lod] * _indOffsetH[lod]);
	GFX_DEVICE.renderBuffer(vbo);

	return 1;
}

void  TerrainChunk::DrawGrass(I8 lod, F32 d,U32 geometryIndex, Transform* const parentTransform){
	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod != 0) return;
	if(d > _grassData._grassVisibility) { //if we go beyond grass visibility limit ...
		return; // ... do not draw grass
	}

	if(!_grassData._grassIndice[geometryIndex].empty())	{
		F32 ratio = 1.0f - (d / _grassData._grassVisibility);
		ratio *= 2.0f;
		if(ratio > 1.0f) ratio = 1.0f;

        U32 indices_count = (U32)( ratio * _grassData._grassIndice[geometryIndex].size() );
		indices_count -= indices_count%4;

		if(indices_count > 0)
			_grassData._grassVBO->setFirstElement(&(_grassData._grassIndice[geometryIndex].front()));
			_grassData._grassVBO->setRangeCount(indices_count);
            GFX_DEVICE.renderBuffer(_grassData._grassVBO,parentTransform);
	}
}