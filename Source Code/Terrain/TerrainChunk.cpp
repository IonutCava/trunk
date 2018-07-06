#include "TerrainChunk.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"
#include "Managers/SceneManager.h"
#include "Managers/ResourceManager.h"
#include "EngineGraphs/SceneGraph.h"

void TerrainChunk::Load(U32 depth, ivec2 pos, ivec2 HMsize){

	for(U32 i=0; i < TERRAIN_CHUNKS_LOD; i++)
		ComputeIndicesArray(i, depth, pos, HMsize);
}


void TerrainChunk::addTree(const vec3& pos,F32 rotation,F32 scale, const std::string& tree_shader, const FileData& tree, SceneGraphNode* parentNode){
	stringstream ss;
	ss << "_" << parentNode->getChildren().size();
	ResourceDescriptor model(tree.ItemName+ss.str());
	model.setResourceLocation(tree.ModelName);
	Mesh* tempTree = ResourceManager::getInstance().LoadResource<Mesh>(model);

	if(tempTree){
		SceneGraphNode* treeGraphNode = parentNode->addNode(tempTree);
		Transform* treeTransform = treeGraphNode->getNode()->getTransform();
		Material*  treeMaterial = treeGraphNode->getNode()->getMaterial();
 		treeTransform->scale(scale * tree.scale);
		treeTransform->rotateY(rotation);
		treeTransform->translate(pos);
		treeMaterial->setShader(tree_shader);
		_trees.push_back(tempTree->getName());
	}else{
		Console::getInstance().errorf("Can't add tree: %s\n",tree.ModelName.c_str());
	}
	ss.clear();
}

void TerrainChunk::ComputeIndicesArray(U32 lod, U32 depth, ivec2 pos, ivec2 HMsize){

	assert(lod < TERRAIN_CHUNKS_LOD);

	ivec2 vHeightmapDataPos = pos;
	U32 offset = (U32)pow(2.0f, (F32)(lod));
	U32 div = (U32)pow(2.0f, (F32)(depth+lod));
	ivec2 vHeightmapDataSize = HMsize/(div) + ivec2(1,1);

	U32 nHMWidth = (U32)vHeightmapDataSize.x;
	U32 nHMHeight = (U32)vHeightmapDataSize.y;
	U32 nHMOffsetX = (U32)vHeightmapDataPos.x;
	U32 nHMOffsetY = (U32)vHeightmapDataPos.y;

	U32 nHMTotalWidth = (U32)HMsize.x;
	U32 nHMTotalHeight = (U32)HMsize.y;

	_indOffsetW[lod] = nHMWidth*2;
	_indOffsetH[lod] = nHMWidth-1;
	U32 nIndice = (nHMWidth)*(nHMHeight-1)*2;
	_indice[lod].reserve( nIndice );


	for(U32 j=0; j<(U32)nHMHeight-1; j++)
	{
		for(U32 i=0; i<(U32)nHMWidth; i++)
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
	_trees.clear();

}

void TerrainChunk::DrawTrees(U32 lod, F32 d,bool drawInReflexion){

	F32 treeVisibility = SceneManager::getInstance().getActiveScene()->getTreeVisibility();
	SceneGraph* sceneGraph = SceneManager::getInstance().getActiveScene()->getSceneGraph();
	assert(lod < TERRAIN_CHUNKS_LOD);
	for(U16 i = 0; i < _trees.size(); i++){
		SceneGraphNode* currentTree = sceneGraph->findNode(_trees[i]);
		assert(currentTree);
		if(d > treeVisibility && !GFXDevice::getInstance().getDepthMapRendering())
			currentTree->setActive(false);
		else{
			currentTree->restoreActive();
			//ToDo: Fix upside-down object water rendering - Ionut
			/*F32 yScale = currentTree->getNode<Mesh>()->getTransform()->getScale().y;
			if( (drawInReflexion && yScale > 0) || (!drawInReflexion && yScale < 0) ) yScale = -yScale;
			currentTree->getNode<Mesh>()->getTransform()->scaleY(yScale);*/

				
		}
	}

}

int TerrainChunk::DrawGround(U32 lod, bool drawInReflexion){

	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod>0) lod--;

	for(U32 j=0; j < _indOffsetH[lod]; j++)
		GFXDevice::getInstance().renderElements(TRIANGLE_STRIP,_indOffsetW[lod],&(_indice[lod][j*_indOffsetW[lod]]),drawInReflexion);

	return 1;
}

void  TerrainChunk::DrawGrass(U32 lod, F32 d, bool drawInReflexion){

	
	F32 grassVisibility = SceneManager::getInstance().getActiveScene()->getGrassVisibility();
	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod != 0) return;
	if(d > grassVisibility && !GFXDevice::getInstance().getDepthMapRendering()) return;
	
	if(_grassIndice.size() > 0)
	{
		F32 ratio = 1.0f - (d / grassVisibility);
		ratio *= 2.0f;
		if(ratio > 1.0f) ratio = 1.0f;

		U32 indices_count = (U32)( ratio * _grassIndice.size() );
		indices_count -= indices_count%4; 

		if(indices_count > 0)
			GFXDevice::getInstance().renderElements(QUADS,indices_count, &(_grassIndice[0]),drawInReflexion);
			
	}

}





