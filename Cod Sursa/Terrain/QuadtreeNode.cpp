#include "QuadtreeNode.h"
#include "Rendering/Frustum.h"
#include "resource.h"

using namespace std;

void QuadtreeNode::Build(U32 depth,		
						 ivec2 pos,					
						 ivec2 HMsize,				
						 U32 minHMSize)	
{
	m_nLOD = 0;

	U32 div = (U32)pow(2.0f, (F32)depth);
	ivec2 nodesize = HMsize/(div);
	if(nodesize.x%2==0) nodesize.x++;
	if(nodesize.y%2==0) nodesize.y++;
	ivec2 newsize = nodesize/2;


	
	if((U32)max(newsize.x, newsize.y) < minHMSize)
	{
		
		m_pTerrainChunk = new TerrainChunk();
		m_pTerrainChunk->Load(depth, pos, HMsize);

		
		m_pChildren = NULL;
		return;
	}



	//Cream 4 "copii"
	m_pChildren = new QuadtreeNode[4];

	// Calculam Bounding Box-ul "copiilor"
	vec3 center = terrain_BBox.getCenter();
	m_pChildren[CHILD_NW].setBoundingBox( BoundingBox(terrain_BBox.min, center) );
	m_pChildren[CHILD_NE].setBoundingBox( BoundingBox(vec3(center.x, 0.0f, terrain_BBox.min.z), vec3(terrain_BBox.max.x, 0.0f, center.z)) );
	m_pChildren[CHILD_SW].setBoundingBox( BoundingBox(vec3(terrain_BBox.min.x, 0.0f, center.z), vec3(center.x, 0.0f, terrain_BBox.max.z)) );
	m_pChildren[CHILD_SE].setBoundingBox( BoundingBox(center, terrain_BBox.max) );

	// Calculam pozitia copiilor
	ivec2 tNewHMpos[4];
	tNewHMpos[CHILD_NW] = pos + ivec2(0, 0);
	tNewHMpos[CHILD_NE] = pos + ivec2(newsize.x, 0);
	tNewHMpos[CHILD_SW] = pos + ivec2(0, newsize.y);
	tNewHMpos[CHILD_SE] = pos + ivec2(newsize.x, newsize.y);


	
	for(int i=0; i<4; i++)
		m_pChildren[i].Build(depth+1, tNewHMpos[i], HMsize, minHMSize);
}

void QuadtreeNode::ComputeBoundingBox(const vec3* vertices)
{
	assert(vertices);

	terrain_BBox.min.y = 100000.0f;
	terrain_BBox.max.y = -100000.0f;

	if(m_pTerrainChunk) {
		std::vector<GLuint>& tIndices = m_pTerrainChunk->getIndiceArray(0);

		for(GLuint i=0; i<tIndices.size(); i++) {
			vec3 vertex = vertices[ tIndices[i] ];

			if(vertex.y > terrain_BBox.max.y)	terrain_BBox.max.y = vertex.y;
			if(vertex.y < terrain_BBox.min.y)	terrain_BBox.min.y = vertex.y;
		}

		for(GLuint i=0; i<m_pTerrainChunk->getTreeArray().size(); i++) {
			DVDFile* obj = ((DVDFile*)m_pTerrainChunk->getTreeArray()[ i ].geometry);
			BoundingBox bbox = obj->getBoundingBox();
			bbox.Translate( obj->getPosition() );
			terrain_BBox.Add( bbox );
		}
	}


	if(m_pChildren) {
		for(int i=0; i<4; i++) {
			m_pChildren[i].ComputeBoundingBox(vertices);

			if(terrain_BBox.min.y > m_pChildren[i].terrain_BBox.min.y)	terrain_BBox.min.y = m_pChildren[i].terrain_BBox.min.y;
			if(terrain_BBox.max.y < m_pChildren[i].terrain_BBox.max.y)	terrain_BBox.max.y = m_pChildren[i].terrain_BBox.max.y;
		}
	}
}


void QuadtreeNode::Destroy()
{
	if(m_pChildren) {
		for(int i=0; i<4; i++)
			m_pChildren[i].Destroy();
		delete [] m_pChildren;
		m_pChildren = NULL;
	}
}

void QuadtreeNode::DrawGrass(bool drawInReflexion)
{
	if(!m_pChildren) {
		assert(m_pTerrainChunk);
		if( m_nLOD>=0 )
			m_pTerrainChunk->DrawGrass( (GLuint)m_nLOD, m_fDistance );
		else
			return;
	}
	else {
		int ret = 0;
		if( m_nLOD>=0 )
			for(int i=0; i<4; i++)
				m_pChildren[i].DrawGrass(drawInReflexion);
		return;		
	}
}

void QuadtreeNode::DrawTrees(bool drawInReflexion)
{
	//DrawBBox(true);
	if(!m_pChildren) {
		assert(m_pTerrainChunk);
		if( m_nLOD>=0 )
			m_pTerrainChunk->DrawTrees(drawInReflexion ? TERRAIN_CHUNKS_LOD-1 : (GLuint)m_nLOD , m_fDistance );
		else
			return;
	}
	else {
		int ret = 0;
		if( m_nLOD>=0 )
			for(int i=0; i<4; i++)
				m_pChildren[i].DrawTrees(drawInReflexion);
		return;		
	}
}

int QuadtreeNode::DrawObjects(bool drawInReflexion)
{
	if(!m_pChildren) {
		assert(m_pTerrainChunk);
		if( m_nLOD>=0 )
			return m_pTerrainChunk->DrawObjects(drawInReflexion ? TERRAIN_CHUNKS_LOD-1 : (GLuint)m_nLOD );
		else
			return 0;
	}
	else {
		int ret = 0;
		if( m_nLOD>=0 )
			for(int i=0; i<4; i++)
				ret += m_pChildren[i].DrawObjects(drawInReflexion);
		return ret;		
	}
}





void QuadtreeNode::DrawBBox(bool bTest)
{
	if(!m_pChildren) {
		assert(m_pTerrainChunk);

		glBegin(GL_LINE_LOOP);
		glVertex3f( terrain_BBox.min.x, terrain_BBox.min.y, terrain_BBox.min.z );
		glVertex3f( terrain_BBox.max.x, terrain_BBox.min.y, terrain_BBox.min.z );
		glVertex3f( terrain_BBox.max.x, terrain_BBox.min.y, terrain_BBox.max.z );
		glVertex3f( terrain_BBox.min.x, terrain_BBox.min.y, terrain_BBox.max.z );
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f( terrain_BBox.min.x, terrain_BBox.max.y, terrain_BBox.min.z );
		glVertex3f( terrain_BBox.max.x, terrain_BBox.max.y, terrain_BBox.min.z );
		glVertex3f( terrain_BBox.max.x, terrain_BBox.max.y, terrain_BBox.max.z );
		glVertex3f( terrain_BBox.min.x, terrain_BBox.max.y, terrain_BBox.max.z );
		glEnd();

		glBegin(GL_LINES);
		glVertex3f( terrain_BBox.min.x, terrain_BBox.min.y, terrain_BBox.min.z );
		glVertex3f( terrain_BBox.min.x, terrain_BBox.max.y, terrain_BBox.min.z );
		glVertex3f( terrain_BBox.max.x, terrain_BBox.min.y, terrain_BBox.min.z );
		glVertex3f( terrain_BBox.max.x, terrain_BBox.max.y, terrain_BBox.min.z );
		glVertex3f( terrain_BBox.max.x, terrain_BBox.min.y, terrain_BBox.max.z );
		glVertex3f( terrain_BBox.max.x, terrain_BBox.max.y, terrain_BBox.max.z );
		glVertex3f( terrain_BBox.min.x, terrain_BBox.min.y, terrain_BBox.max.z );
		glVertex3f( terrain_BBox.min.x, terrain_BBox.max.y, terrain_BBox.max.z );
		glEnd();
		
	}
	else {
		if( m_nLOD>=0 )
			for(int i=0; i<4; i++)
				m_pChildren[i].DrawBBox(bTest);
	}
}


int QuadtreeNode::DrawGround(Frustum* pFrust, int options)
{
	assert(pFrust);

	m_nLOD = -1;

	vec3 center = terrain_BBox.getCenter();				
	F32 radius = (terrain_BBox.max-center).length();	

	if(options & CHUNK_BIT_TESTCHILDREN) {
		if(!terrain_BBox.ContainsPoint(pFrust->getEyePos()))
		{
			int resSphereInFrustum = pFrust->ContainsSphere(center, radius);
			switch(resSphereInFrustum) {
				case FRUSTUM_OUT: return 0;		
				case FRUSTUM_IN:
					options &= ~CHUNK_BIT_TESTCHILDREN;				
					break;		
				case FRUSTUM_INTERSECT:								
				{		
					int resBoxInFrustum = pFrust->ContainsBoundingBox(terrain_BBox);
					switch(resBoxInFrustum) {
						case FRUSTUM_IN: options &= ~CHUNK_BIT_TESTCHILDREN; break;
						case FRUSTUM_OUT: return 0;
					}
				}
			}
		}
	}

	m_nLOD = 0; 

	if(!m_pChildren) {
		assert(m_pTerrainChunk);

		if(options & CHUNK_BIT_WATERREFLECTION) {
			return m_pTerrainChunk->DrawGround(TERRAIN_CHUNKS_LOD-1);
		}
		else {
			
			vec3 vEyeToChunk = this->getBoundingBox().getCenter() - pFrust->getEyePos();
			m_fDistance = vEyeToChunk.length();
			GLuint lod = 0;
			if(m_fDistance > TERRAIN_CHUNK_LOD1)		lod = 2;
			else if(m_fDistance > TERRAIN_CHUNK_LOD0)	lod = 1;
			m_nLOD = lod;

			return m_pTerrainChunk->DrawGround(lod);
		}
	}
	else {
		int ret = 0;
		for(int i=0; i<4; i++)
			ret += m_pChildren[i].DrawGround(pFrust, options);
		return ret;
	}

}


