#include "Headers/TerrainChunk.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Geometry/Shapes/Headers/Mesh.h" 

typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;
void TerrainChunk::Load(U8 depth, vec2<U32> pos, vec2<U32> HMsize){

	for(U8 i=0; i < TERRAIN_CHUNKS_LOD; i++)
		ComputeIndicesArray(i, depth, pos, HMsize);
	_grassVisibility = GET_ACTIVE_SCENE()->state()->getGrassVisibility();
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
		
	}else{
		ERROR_FN(Locale::get("ERROR_ADD_TREE"),tree.ModelName.c_str());
	}
}

void TerrainChunk::ComputeIndicesArray(I8 lod, U8 depth, vec2<U32> pos, vec2<U32> HMsize){

	assert(lod < TERRAIN_CHUNKS_LOD);

	vec2<U32> vHeightmapDataPos = pos;
	U32 offset = (U32)pow(2.0f, (F32)(lod));
	U32 div = (U32)pow(2.0f, (F32)(depth+lod));
	vec2<U32> vHeightmapDataSize = HMsize/(div) + vec2<U32>(1,1);

	U32 nHMWidth   = vHeightmapDataSize.x;
	U32 nHMHeight  = vHeightmapDataSize.y;
	U32 nHMOffsetX = vHeightmapDataPos.x;
	U32 nHMOffsetY = vHeightmapDataPos.y;

	U32 nHMTotalWidth  = HMsize.x;
	U32 nHMTotalHeight = HMsize.y;

	_indOffsetW[lod] = nHMWidth*2;
	_indOffsetH[lod] = nHMWidth-1;
	U32 nIndice = (nHMWidth)*(nHMHeight-1)*2;
	_indice[lod].reserve( nIndice );


	for(U16 j=0; j<nHMHeight-1; j++){
		for(U16 i=0; i<nHMWidth; i++){
			U32 id0 = (j*(offset) + nHMOffsetY+0)*(nHMTotalWidth)+(i*(offset) + nHMOffsetX+0);
			U32 id1 = (j*(offset) + nHMOffsetY+(offset))*(nHMTotalWidth)+(i*(offset) + nHMOffsetX+0);
			_indice[lod].push_back( id0 );
			_indice[lod].push_back( id1 );
		}
	}

	assert(nIndice == _indice[lod].size());
}


void TerrainChunk::Destroy(){

	for(U8 i=0; i<TERRAIN_CHUNKS_LOD; i++){
		_indice[i].clear();
		_indOffsetW[i] = 0;
		_indOffsetH[i] = 0;
		_grassVisibility = 0;
	}
	_grassIndice.clear();
}

int TerrainChunk::DrawGround(I8 lod, ShaderProgram* const program, bool drawInReflection){

	assert(lod < TERRAIN_CHUNKS_LOD);

	if(lod>0) lod--;
	program->Uniform("LOD", lod+2);

	for(U16 j=0; j < _indOffsetH[lod]; j++){
		GFX_DEVICE.renderElements(TRIANGLE_STRIP,UNSIGNED_INT,_indOffsetW[lod],&(_indice[lod][j*_indOffsetW[lod]]));
	}

	return 1;
}

void  TerrainChunk::DrawGrass(I8 lod, F32 d){

	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod != 0) return;
	if(d > _grassVisibility) { //if we go beyond grass visibility limit ...
			return; // ... do not draw grass
	}
	
	if(!_grassIndice.empty())	{
		F32 ratio = 1.0f - (d / _grassVisibility);
		ratio *= 2.0f;
		if(ratio > 1.0f) ratio = 1.0f;

		U32 indices_count = (U32)( ratio * _grassIndice.size() );
		indices_count -= indices_count%4; 

		if(indices_count > 0)
			GFX_DEVICE.renderElements(QUADS,UNSIGNED_SHORT,indices_count, &(_grassIndice[0]));
			
	}

}





