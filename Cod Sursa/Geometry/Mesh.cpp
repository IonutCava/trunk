#include "Mesh.h"
#include "TextureManager/Texture2D.h"
#include "Managers/ResourceManager.h"
#include "Rendering/Frustum.h"
#include "Managers/SceneManager.h"

bool Mesh::isVisible()
{
	if(!_render || !isInView() || _subMeshes.empty())
		return false;
	_bb.setVisibility(_selected); 
	DrawBBox();
	return true;

}

void Mesh::DrawBBox()
{
	if(!_render || !_bb.getVisibility()) return;
	if(!_bb.isComputed()) computeBoundingBox();
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

bool Mesh::isInView()
{
	if(!_render) return false;
	if(!_bb.isComputed()) computeBoundingBox();
	//return true; 

	vec3 vEyeToChunk = getBoundingBox().getCenter() - Frustum::getInstance().getEyePos();

	// Bellow code is still buggy. ToDo: FIX THIS!!!!!!!
	//if(vEyeToChunk.length() > SceneManager::getInstance().getTerrainManager()->getGeneralVisibility()) return false;
	// END BUGGY CODE :P

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
	if(!_render) return;
	getPosition() = position;
	if(!_bb.isComputed()) computeBoundingBox();
	_bb.Translate(position);

}

void Mesh::setScale(vec3 scale)
{
	if(!_render) return;
	getScale() = scale;
	if(!_bb.isComputed()) computeBoundingBox();
	_bb.Multiply(scale);
}

BoundingBox& Mesh::getBoundingBox()
{
	return _bb;
}

void Mesh::computeBoundingBox()
{
	_bb.min = vec3(100000.0f, 100000.0f, 100000.0f);
	_bb.max = vec3(-100000.0f, -100000.0f, -100000.0f);

	for(_subMeshIterator = _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
		for(int i=0; i<(int)(*_subMeshIterator)->getGeometryVBO()->getPosition().size(); i++)
		{
			vec3& position = (*_subMeshIterator)->getGeometryVBO()->getPosition()[i];
			_bb.Add(position);
		}
	_bb.isComputed() = true;
}

SubMesh*  Mesh::getSubMesh(const string& name)
{
	for(_subMeshIterator =  _subMeshes.begin(); _subMeshIterator != _subMeshes.end(); _subMeshIterator++)
		if((*_subMeshIterator)->getName().compare(name) == 0)
			return (*_subMeshIterator);
	return NULL;
}