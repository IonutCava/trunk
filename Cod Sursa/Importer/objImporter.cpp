#include "OBJ.h"
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/Frustum.h"
#include "Managers/TerrainManager.h"
#include "Managers/SceneManager.h"

bool ImportedModel::ReadOBJ(const std::string& filename)
{
	ListID = -1;
    FILE*   file;
	
	fopen_s(&file,filename.c_str(),"r");

	if (!file) {cout << "ImportedModel: Could not load model [ " << filename << " ] " << endl; return false; }

    pathname    =   filename.substr( 0, filename.rfind("/")+1 );
    mtllibname    = "";
    getName() = ModelName = filename.substr(filename.rfind("/")+1,filename.length());

	OBJ::glmFirstPass(this, file);
    OBJ::glmSecondPass(this, file);
	return true;
}

void ImportedModel::DrawBBox()
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
bool ImportedModel::IsInView()
{
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
void ImportedModel::Draw()
{
	if(getName().find(".obj") != string::npos)
	{
		
		//OBJ::glmDraw(this, GLM_SMOOTH | GLM_TEXTURE | GLM_MATERIAL);
		if(ListID == -1) ListID = OBJ::glmList(this,GLM_SMOOTH | GLM_TEXTURE | GLM_MATERIAL);
		else glCallList(ListID);
	}
/*	else if(getName().find(".dae") != string::npos)
		COLLADA::Draw(this);
	else if(getName().find(".3ds") != string::npos)
		3DS::Draw(this);
	else
		DIVIDE::Draw(this);
*/
}


bool ImportedModel::unload()
{
	textures.clear(); 
	_shader->unload();
	delete _shader;
	groups.clear();
	materials.clear();
	SimpleFaceArray.clear();
	triangles.clear();
	facetnorms.clear();
	texcoords.clear();
	normals.clear();
	vertices.clear();
	mtllibname.clear();
	pathname.clear();
	
	return true;
}

void ImportedModel::setPosition(vec3 position)
{
	getPosition() = position;
	getBoundingBox().Translate(position - getBoundingBox().getCenter());
}
