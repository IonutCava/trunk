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
	
	cout << "Adding tree [ " << i <<  " ] : " << DA[i].ModelName << endl;
	Tree t;
	t.geometry = ResourceManager::getInstance().LoadResource<DVDFile>(DA[i].ModelName);

	if(t.geometry)
	{
		t.position = pos;
		t.orientation = vec3(0,rotation,0);
		t.scale = scale*DA[i].scale;
		t.name = "Tree " + m_tTrees.size();
		((DVDFile*)t.geometry)->setShader(ResourceManager::getInstance().LoadResource<Shader>("terrain_tree"));
	}
	m_tTrees.push_back(t);

}

void TerrainChunk::ComputeIndicesArray(U32 lod, U32 depth, ivec2 pos, ivec2 HMsize)
{
	assert(lod < TERRAIN_CHUNKS_LOD);

	ivec2 vHeightmapDataPos = pos;
	U32 offset = (U32)pow(2.0f, (F32)(lod));
	U32 div = (U32)pow(2.0f, (F32)(depth+lod));
	ivec2 vHeightmapDataSize = HMsize/(div) + ivec2(1,1);

	GLuint nHMWidth = (GLuint)vHeightmapDataSize.x;
	GLuint nHMHeight = (GLuint)vHeightmapDataSize.y;
	GLuint nHMOffsetX = (GLuint)vHeightmapDataPos.x;
	GLuint nHMOffsetY = (GLuint)vHeightmapDataPos.y;

	GLuint nHMTotalWidth = (GLuint)HMsize.x;
	GLuint nHMTotalHeight = (GLuint)HMsize.y;

	m_tIndOffsetW[lod] = nHMWidth*2;
	m_tIndOffsetH[lod] = nHMWidth-1;
	GLuint nIndice = (nHMWidth)*(nHMHeight-1)*2;
	m_tIndice[lod].reserve( nIndice );


	for(GLuint j=0; j<(GLuint)nHMHeight-1; j++)
	{
		for(GLuint i=0; i<(GLuint)nHMWidth; i++)
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
	for(GLuint i=0; i<TERRAIN_CHUNKS_LOD; i++)
		m_tIndice[i].clear();

	for(int i=0; i<(int)m_tObject.size(); i++)
		delete m_tObject[i];
}


int TerrainChunk::DrawObjects(GLuint lod)
{
	assert(lod < TERRAIN_CHUNKS_LOD);

	for(int i=0; i<(int)m_tObject.size(); i++)
		m_tObject[i]->Draw();

	return (int)m_tObject.size();
}

void TerrainChunk::DrawTrees(GLuint lod, F32 d)
{
	if(d > SceneManager::getInstance().getTerrainManager()->getTreeVisibility()) return;
	F32 _windX = SceneManager::getInstance().getTerrainManager()->getWindDirX();
	F32 _windZ = SceneManager::getInstance().getTerrainManager()->getWindDirX();
	F32 _windS = SceneManager::getInstance().getTerrainManager()->getWindSpeed();
	
	assert(lod < TERRAIN_CHUNKS_LOD);
	
	for(int i=0; i<(int)m_tTrees.size(); i++)
	{
		GFXDevice::getInstance().pushMatrix();
		GFXDevice::getInstance().translate(m_tTrees[i].position);
		GFXDevice::getInstance().rotate(m_tTrees[i].orientation);
		GFXDevice::getInstance().scale(m_tTrees[i].scale);
		((DVDFile*)m_tTrees[i].geometry)->getShader()->bind();

		((DVDFile*)m_tTrees[i].geometry)->getShader()->Uniform("time", GETTIME());
		((DVDFile*)m_tTrees[i].geometry)->getShader()->Uniform("scale", m_tTrees[i].scale.y);
		((DVDFile*)m_tTrees[i].geometry)->getShader()->Uniform("windDirectionX", _windX);
		((DVDFile*)m_tTrees[i].geometry)->getShader()->Uniform("windDirectionZ", _windZ);
		((DVDFile*)m_tTrees[i].geometry)->getShader()->Uniform("windSpeed", _windS);
			((DVDFile*)m_tTrees[i].geometry)->Draw();

		((DVDFile*)m_tTrees[i].geometry)->getShader()->unbind();
		GFXDevice::getInstance().popMatrix();
	}
}

int TerrainChunk::DrawGround(GLuint lod)
{
	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod>0) lod--;

	for(GLuint j=0; j<(GLuint)m_tIndOffsetH[lod]; j++)
		glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)m_tIndOffsetW[lod], GL_UNSIGNED_INT, &(m_tIndice[lod][j*m_tIndOffsetW[lod]]) );

	return 1;
}

void  TerrainChunk::DrawGrass(GLuint lod, F32 d)
{
	
	F32 grassVisibility = SceneManager::getInstance().getTerrainManager()->getGrassVisibility();
	assert(lod < TERRAIN_CHUNKS_LOD);
	if(lod != 0) return;
	if(d > grassVisibility) return;

	if(m_tGrassIndice.size() > 0)
	{
		F32 ratio = 1.0f - (d / grassVisibility);
		ratio *= 2.0f;
		if(ratio > 1.0f) ratio = 1.0f;

		GLsizei indices_count = (GLsizei)( ratio * m_tGrassIndice.size() );
		indices_count -= indices_count%4; 

		if(indices_count > 0)
			glDrawElements(GL_QUADS, indices_count, GL_UNSIGNED_INT, &(m_tGrassIndice[0]) );
	}

}





