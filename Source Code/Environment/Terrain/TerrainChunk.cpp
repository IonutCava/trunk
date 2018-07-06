#include "Headers/TerrainChunk.h"
#include "Utility/Headers/Guardian.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Geometry/Shapes/Headers/Mesh.h" 

typedef unordered_map<std::string, SceneGraphNode*> NodeChildren;
void TerrainChunk::Load(U8 depth, ivec2 pos, ivec2 HMsize){

	for(U8 i=0; i < TERRAIN_CHUNKS_LOD; i++)
		ComputeIndicesArray(i, depth, pos, HMsize);
	_grassVisibility = SceneManager::getInstance().getActiveScene()->getGrassVisibility();
}


void TerrainChunk::addTree(const vec4& pos,F32 scale, const FileData& tree, SceneGraphNode* parentNode){
	ResourceDescriptor model(tree.ItemName);
	model.setResourceLocation(tree.ModelName);
	model.setFlag(true);
	Mesh* tempTree = ResourceManager::getInstance().loadResource<Mesh>(model);
	if(tempTree){
		stringstream ss; ss << "_" << tempTree->getRefCount();
		std::string treeName(tempTree->getName()+ss.str());
		SceneGraphNode* treeNode = parentNode->addNode(tempTree,treeName);
		Console::getInstance().printfn("Added tree [ %s ]",treeNode->getName().c_str());
		Transform* treeTransform = treeNode->getTransform();
 		treeTransform->scale(scale * tree.scale);
		treeTransform->rotateY(pos.w);
		treeTransform->translate(vec3(pos));
		for_each(SceneGraphNode::NodeChildren::value_type& it, treeNode->getChildren()){
			assert(it.second);
			Material* m = (it.second)->getNode()->getMaterial();
			if(m){
				m->setShader("terrain_tree.vert,lighting_texture.frag");
			}
		}
		
	}else{
		Console::getInstance().errorf("Can't add tree: %s\n",tree.ModelName.c_str());
	}
}

void TerrainChunk::ComputeIndicesArray(I8 lod, U8 depth, ivec2 pos, ivec2 HMsize){

	assert(lod < TERRAIN_CHUNKS_LOD);

	ivec2 vHeightmapDataPos = pos;
	U32 offset = (U32)pow(2.0f, (F32)(lod));
	U32 div = (U32)pow(2.0f, (F32)(depth+lod));
	ivec2 vHeightmapDataSize = HMsize/(div) + ivec2(1,1);

	U16 nHMWidth = (U16)vHeightmapDataSize.x;
	U16 nHMHeight = (U16)vHeightmapDataSize.y;
	U32 nHMOffsetX = (U32)vHeightmapDataPos.x;
	U32 nHMOffsetY = (U32)vHeightmapDataPos.y;

	U32 nHMTotalWidth = (U32)HMsize.x;
	U32 nHMTotalHeight = (U32)HMsize.y;

	_indOffsetW[lod] = nHMWidth*2;
	_indOffsetH[lod] = nHMWidth-1;
	U32 nIndice = (nHMWidth)*(nHMHeight-1)*2;
	_indice[lod].reserve( nIndice );


	for(U16 j=0; j<nHMHeight-1; j++)
	{
		for(U16 i=0; i<nHMWidth; i++)
		{
			U32 id0 = (j*(offset) + nHMOffsetY+0)*(nHMTotalWidth)+(i*(offset) + nHMOffsetX+0);
			U32 id1 = (j*(offset) + nHMOffsetY+(offset))*(nHMTotalWidth)+(i*(offset) + nHMOffsetX+0);
			_indice[lod].push_back( id0 );
			_indice[lod].push_back( id1 );
		}
	}

	assert(nIndice == _indice[lod].size());
}


void TerrainChunk::Destroy(){

	for(U8 i=0; i<TERRAIN_CHUNKS_LOD; i++)
		_indice[i].clear();
}

int TerrainChunk::DrawGround(I8 lod, bool drawInReflection){

	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod>0) lod--;

	for(U16 j=0; j < _indOffsetH[lod]; j++){
		_gfx.renderElements(TRIANGLE_STRIP,_U32,_indOffsetW[lod],&(_indice[lod][j*_indOffsetW[lod]]));
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
			_gfx.renderElements(QUADS,_U16,indices_count, &(_grassIndice[0]));
			
	}

}





