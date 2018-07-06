#include "TerrainChunk.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"
#include "Managers/SceneManager.h"
#include "Managers/TerrainManager.h"
#include "Geometry/Object3DFlyWeight.h"


void TerrainChunk::Load(U32 depth, ivec2 pos, ivec2 HMsize)
{
	for(U32 i=0; i < TERRAIN_CHUNKS_LOD; i++)
		ComputeIndicesArray(i, depth, pos, HMsize);
}


void TerrainChunk::addTree(const vec3& pos,F32 rotation,F32 scale, const std::string& tree_shader, const FileData& tree)
{
	Mesh* t = ResourceManager::getInstance().LoadResource<Mesh>(tree.ModelName);
	if(t)
	{	
		t->getMaterial().setShader(ResourceManager::getInstance().LoadResource<Shader>(tree_shader));
		t->getMaterial().getShader()->bind();
			t->getMaterial().getShader()->Uniform("enable_shadow_mapping", 0);
			t->getMaterial().getShader()->Uniform("tile_factor", 1.0f);	
		t->getMaterial().getShader()->unbind();
		Object3DFlyWeight* tempTree = New Object3DFlyWeight(t);
		Transform* tran = tempTree->getTransform();
		tran->scale(scale * tree.scale);
		tran->rotateY(rotation);
		tran->setPosition(pos);
		_trees.push_back(tempTree);
	}
	else
	{
		Con::getInstance().errorf("Can't add tree: %s\n",tree.ModelName);
	}

}

void TerrainChunk::ComputeIndicesArray(U32 lod, U32 depth, ivec2 pos, ivec2 HMsize)
{
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


void TerrainChunk::Destroy()
{
	for(U8 i=0; i<TERRAIN_CHUNKS_LOD; i++)
		_indice[i].clear();
	for(U8 i=0; i<_trees.size(); i++)
		delete _trees[i];

}

void TerrainChunk::DrawTrees(U32 lod, F32 d)
{
	F32 treeVisibility = SceneManager::getInstance().getTerrainManager()->getTreeVisibility();
	assert(lod < TERRAIN_CHUNKS_LOD);
	if(d > treeVisibility && !GFXDevice::getInstance().getDepthMapRendering()) return;

	F32 _windX = SceneManager::getInstance().getTerrainManager()->getWindDirX();
	F32 _windZ = SceneManager::getInstance().getTerrainManager()->getWindDirX();
	F32 _windS = SceneManager::getInstance().getTerrainManager()->getWindSpeed();

	
	F32 time = GETTIME();
	Shader* treeShader;
	for(U32 i=0; i <  _trees.size(); i++)
	{
		treeShader = _trees[i]->getObject()->getMaterial().getShader();
		if(!treeShader) continue;
		treeShader->bind();
			treeShader->Uniform("time", time);
			treeShader->Uniform("scale", _trees[i]->getTransform()->getScale().y);
			treeShader->Uniform("windDirectionX", _windX);
			treeShader->Uniform("windDirectionZ", _windZ);
			treeShader->Uniform("windSpeed", _windS);
		treeShader->unbind();
	}
	GFXDevice::getInstance().renderElements(_trees);
}

int TerrainChunk::DrawGround(U32 lod)
{
	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod>0) lod--;

	for(U32 j=0; j < _indOffsetH[lod]; j++)
		GFXDevice::getInstance().renderElements(TRIANGLE_STRIP,_indOffsetW[lod],&(_indice[lod][j*_indOffsetW[lod]]));

	return 1;
}

void  TerrainChunk::DrawGrass(U32 lod, F32 d)
{
	
	F32 grassVisibility = SceneManager::getInstance().getTerrainManager()->getGrassVisibility();
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
			GFXDevice::getInstance().renderElements(QUADS,indices_count, &(_grassIndice[0]));
			
	}

}





