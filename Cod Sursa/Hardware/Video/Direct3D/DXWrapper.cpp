#include "DXWrapper.h"
#include "Utility/Headers/DataTypes.h"
#include "Importer/DVDConverter.h"

#include <iostream>
void DX_API::initHardware()
{
	Con::getInstance().printfn("Initializing Direct3D rendering API! ");
}

void DX_API::closeRenderingApi()
{
}

void DX_API::lookAt(const vec3& eye,const vec3& center,const vec3& up)
{
}

void DX_API::idle()
{
}

mat4 DX_API::getModelViewMatrix()
{
	mat4 mat;
	return mat;
}

mat4 DX_API::getProjectionMatrix()
{
	mat4 mat;
	return mat;
}

void DX_API::translate(const vec3& pos)
{
}

void DX_API::rotate(F32 angle,const vec3& weights)
{
}


void DX_API::scale(const vec3& scale)
{
}


void DX_API::clearBuffers(int buffer_mask)
{
}

void DX_API::swapBuffers()
{
}

void DX_API::enableFog(F32 density, F32* color)
{
}

void DX_API::enable_MODELVIEW()
{
}

void DX_API::loadIdentityMatrix()
{
}

void DX_API::toggle2D(bool _2D)
{
}

void DX_API::drawTextToScreen(Text* text)
{
}

void DX_API::drawCharacterToScreen(void* ,char)
{
}

void DX_API::drawButton(Button* button)
{
}

void DX_API::drawFlash(GuiFlash* flash)
{
}

void DX_API::drawBox3D(vec3 min, vec3 max)
{
}

void DX_API::drawBox3D(Box3D* const box)
{
}

void DX_API::drawSphere3D(Sphere3D* const sphere)
{
}

void DX_API::drawQuad3D(Quad3D* const quad)
{
}

void DX_API::drawText3D(Text3D* const text)
{
}

void DX_API::renderModel(DVDFile* const model)
{
	if(!model->isVisible()) return;
	
	SubMesh *s;
	vector<SubMesh* >::iterator _subMeshIterator;
	
	//pushMatrix();
	//translate(model->getPosition());
	//rotate(model->getOrientation().x,vec3(1.0f,0.0f,0.0f));
	//rotate(model->getOrientation().y,vec3(0.0f,1.0f,0.0f));
	//rotate(model->getOrientation().z,vec3(0.0f,0.0f,1.0f));
	//scale(model->getScale());
	for(U8 n = 0; n < model->getShaders().size(); n++)
		model->getShaders()[n]->bind();
	
	for(_subMeshIterator = model->getSubMeshes().begin(); 
		_subMeshIterator != model->getSubMeshes().end(); 
		_subMeshIterator++)
	{
		s = (*_subMeshIterator);
		//s->getGeometryVBO()->Enable();
		//s->getMaterial().texture->Bind(0);
			for(U8 n = 0; n < model->getShaders().size(); n++)
				model->getShaders()[n]->UniformTexture("texDiffuse",0);
	
	//		glDrawElements(GL_TRIANGLES, s->getIndices().size(), GL_UNSIGNED_INT, &(s->getIndices()[0]));

		//s->getMaterial().texture->Unbind(0);
		//s->getGeometryVBO()->Disable();
		
	}
	for(U8 n = 0; n < model->getShaders().size(); n++)
		model->getShaders()[n]->unbind();
	//popMatrix();
}

void DX_API::renderElements(Type t, U32 count, const void* first_element)
{
}

void DX_API::setMaterial(Material& mat)
{
}

void DX_API::setColor(const vec4& color)
{
}

void DX_API::setColor(const vec3& color)
{
}


void DX_API::initDevice()
{
}