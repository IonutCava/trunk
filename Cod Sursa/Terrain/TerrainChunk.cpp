//#include "Hardware/Video/OpenGL/glResources.h"
#include "TerrainChunk.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"
#include "Managers/SceneManager.h"
#include "Managers/TerrainManager.h"

void TerrainChunk::Load(U32 depth, ivec2 pos, ivec2 HMsize)
{
	for(U32 i=0; i<TERRAIN_CHUNKS_LOD; i++)
		ComputeIndicesArray(i, depth, pos, HMsize);
}

void TerrainChunk::addObject(DVDFile* obj)
{
	m_tObject.push_back(obj);
}

void TerrainChunk::addTree(vec3 pos,F32 rotation,F32 scale)
{
	vector<FileData> DA = SceneManager::getInstance().getVegetationDataArray();
	int i = random((int)DA.size() - 1);
	
	DVDFile* t = ResourceManager::getInstance().LoadResource<DVDFile>(DA[i].ModelName);
	if(t)
	{
		t->getName() = DA[i].ModelName;
		t->setShader(ResourceManager::getInstance().LoadResource<Shader>("terrain_tree"));
		t->getOrientation() = vec3(0,rotation,0);
		t->setScale(scale*DA[i].scale);
		t->setPosition(pos);
	}

	m_tTrees.push_back(*t);

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

	m_tIndOffsetW[lod] = nHMWidth*2;
	m_tIndOffsetH[lod] = nHMWidth-1;
	U32 nIndice = (nHMWidth)*(nHMHeight-1)*2;
	m_tIndice[lod].reserve( nIndice );


	for(U32 j=0; j<(U32)nHMHeight-1; j++)
	{
		for(U32 i=0; i<(U32)nHMWidth; i++)
		{
			U32 id0 = (j*(offset) + nHMOffsetY+0)*(nHMTotalWidth)+(i*(offset) + nHMOffsetX+0);
			U32 id1 = (j*(offset) + nHMOffsetY+(offset))*(nHMTotalWidth)+(i*(offset) + nHMOffsetX+0);
			m_tIndice[lod].push_back( id0 );
			m_tIndice[lod].push_back( id1 );
		}
	}

	assert(nIndice == m_tIndice[lod].size());
	
}


void TerrainChunk::Destroy()
{
	for(U32 i=0; i<TERRAIN_CHUNKS_LOD; i++)
		m_tIndice[i].clear();

	for(int i=0; i<(int)m_tObject.size(); i++)
	{
		delete m_tObject[i];
		m_tObject[i] = NULL;
	}
}


int TerrainChunk::DrawObjects(U32 lod)
{
	assert(lod < TERRAIN_CHUNKS_LOD);
    RenderState s(true,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	for(int i=0; i<(int)m_tObject.size(); i++)
		GFXDevice::getInstance().renderModel(m_tObject[i]);

	return (int)m_tObject.size();
}

void TerrainChunk::DrawTrees(U32 lod, F32 d)
{
	F32 treeVisibility = SceneManager::getInstance().getTerrainManager()->getTreeVisibility();
	if(d > treeVisibility) return;

	F32 _windX = SceneManager::getInstance().getTerrainManager()->getWindDirX();
	F32 _windZ = SceneManager::getInstance().getTerrainManager()->getWindDirX();
	F32 _windS = SceneManager::getInstance().getTerrainManager()->getWindSpeed();
	
	assert(lod < TERRAIN_CHUNKS_LOD);

	F32 ratio = (1.0f - (d / treeVisibility)) * 2.0f;
	if(ratio > 1.0f) ratio = 1.0f;
	int indices_count = (int)( ratio * m_tTrees.size() );
	//	indices_count -= indices_count%4; 
		
	RenderState s(true,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	if(indices_count > 0)
	for(int i=0; i < indices_count; i++)
	{
		m_tTrees[i].getShader()->bind();
			m_tTrees[i].getShader()->Uniform("time", GETTIME());
			m_tTrees[i].getShader()->Uniform("scale", m_tTrees[i].getScale().y);
			m_tTrees[i].getShader()->Uniform("windDirectionX", _windX);
			m_tTrees[i].getShader()->Uniform("windDirectionZ", _windZ);
			m_tTrees[i].getShader()->Uniform("windSpeed", _windS);
		m_tTrees[i].getShader()->unbind();
		
		GFXDevice::getInstance().renderModel(&m_tTrees[i]);
		
	}
}

int TerrainChunk::DrawGround(U32 lod)
{
	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod>0) lod--;

	for(U32 j=0; j<(U32)m_tIndOffsetH[lod]; j++)
		//glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)m_tIndOffsetW[lod], GL_UNSIGNED_INT, &(m_tIndice[lod][j*m_tIndOffsetW[lod]]) );
		GFXDevice::getInstance().renderElements(TRIANGLE_STRIP,m_tIndOffsetW[lod],&(m_tIndice[lod][j*m_tIndOffsetW[lod]]));

	return 1;
}

void  TerrainChunk::DrawGrass(U32 lod, F32 d)
{
	
	F32 grassVisibility = SceneManager::getInstance().getTerrainManager()->getGrassVisibility();
	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod != 0) return;
	if(d > grassVisibility) return;
	RenderState s(false,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	if(m_tGrassIndice.size() > 0)
	{
		F32 ratio = 1.0f - (d / grassVisibility);
		ratio *= 2.0f;
		if(ratio > 1.0f) ratio = 1.0f;

		int indices_count = (int)( ratio * m_tGrassIndice.size() );
		indices_count -= indices_count%4; 

		if(indices_count > 0)
			GFXDevice::getInstance().renderElements(QUADS,indices_count, &(m_tGrassIndice[0]));
			
	}

}





