#include "Mesh.h"
#include "TextureManager/Texture2D.h"
#include "Managers/ResourceManager.h"
#include "Rendering/Frustum.h"
#include "Managers/SceneManager.h"

void Mesh::Draw()
{
	SubMesh *s;
	
	glPushAttrib(GL_POLYGON_BIT);
	glDisable(GL_CULL_FACE);
	for(_subMeshIterator = _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
	{
		s = (*_subMeshIterator);
		s->getGeometryVBO()->Enable();
		s->getMaterial().texture->Bind(0);
		_shader->UniformTexture("texDiffuse",0);
	
			glDrawElements(GL_TRIANGLES, s->getIndices().size(), GL_UNSIGNED_INT, &(s->getIndices()[0]));

		s->getMaterial().texture->Unbind(0);
		s->getGeometryVBO()->Disable();
	}
	glPopAttrib();
	
}

void Mesh::DrawBBox()
{
	glBegin(GL_LINE_LOOP);
		glVertex3f( _bb.min.x, _bb.min.y, _bb.min.z );
		glVertex3f( _bb.max.x, _bb.min.y, _bb.min.z );
		glVertex3f( _bb.max.x, _bb.min.y, _bb.max.z );
		glVertex3f( _bb.min.x, _bb.min.y, _bb.max.z );
		glEnd();

	glBegin(GL_LINE_LOOP);
		glVertex3f( _bb.min.x, _bb.max.y, _bb.min.z );
		glVertex3f( _bb.max.x, _bb.max.y, _bb.min.z );
		glVertex3f( _bb.max.x, _bb.max.y, _bb.max.z );
		glVertex3f( _bb.min.x, _bb.max.y, _bb.max.z );
	glEnd();

	glBegin(GL_LINES);
		glVertex3f( _bb.min.x, _bb.min.y, _bb.min.z );
		glVertex3f( _bb.min.x, _bb.max.y, _bb.min.z );
		glVertex3f( _bb.max.x, _bb.min.y, _bb.min.z );
		glVertex3f( _bb.max.x, _bb.max.y, _bb.min.z );
		glVertex3f( _bb.max.x, _bb.min.y, _bb.max.z );
		glVertex3f( _bb.max.x, _bb.max.y, _bb.max.z );
		glVertex3f( _bb.min.x, _bb.min.y, _bb.max.z );
		glVertex3f( _bb.min.x, _bb.max.y, _bb.max.z );
	glEnd();
}

bool Mesh::IsInView()
{
	if(!getBoundingBox().isComputed()) computeBoundingBox();
	vec3 vEyeToChunk = getBoundingBox().getCenter() - Frustum::getInstance().getEyePos();
	if(vEyeToChunk.length() > SceneManager::getInstance().getTerrainManager()->getGeneralVisibility()) return false;

	vec3 center = getBoundingBox().getCenter();
	float radius = (getBoundingBox().max-center).length();
	if(!getBoundingBox().ContainsPoint(Frustum::getInstance().getEyePos()))
	{
		switch(Frustum::getInstance().ContainsSphere(center, radius)) {
				case FRUSTUM_OUT: 	return false;
				
				case FRUSTUM_INTERSECT:	
				{		
					if(Frustum::getInstance().ContainsBoundingBox(getBoundingBox()) == FRUSTUM_OUT)	return false;
				}
			}
	}
	return true;
}

void Mesh::setPosition(vec3 position)
{
	getPosition() = position;
	getBoundingBox().Translate(position - getBoundingBox().getCenter());
}

void Mesh::computeBoundingBox()
{
	getBoundingBox().min = vec3(100000.0f, 100000.0f, 100000.0f);
	getBoundingBox().max = vec3(-100000.0f, -100000.0f, -100000.0f);
	for(_subMeshIterator = _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
	for(int i=0; i<(int)(*_subMeshIterator)->getGeometryVBO()->getPosition().size(); i++)
	{
		getBoundingBox().Add((*_subMeshIterator)->getGeometryVBO()->getPosition()[i]);
	}
	getBoundingBox().isComputed() = true;
}
SubMesh*  Mesh::getSubMesh(const string& name)
{
	for(_subMeshIterator =  _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
		if((*_subMeshIterator)->getName().compare(name) == 0)
			return (*_subMeshIterator);
	return NULL;
}