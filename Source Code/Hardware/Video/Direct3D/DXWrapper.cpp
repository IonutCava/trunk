#include "DXWrapper.h"
#include "Hardware/Platform/PlatformDefines.h"
#include "Importer/DVDConverter.h"
#include <iostream>
using namespace std;

void DX_API::initHardware()
{
	Console::getInstance().printfn("Initializing Direct3D rendering API! ");
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


void DX_API::clearBuffers(U8 buffer_mask)
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

void DX_API::setTextureMatrix(U16 slot, const mat4& transformMatrix)
{
}

void DX_API::restoreTextureMatrix(U16 slot)
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

void DX_API::renderModel(Object3D* const model)
{
	Mesh* tempModel = dynamic_cast<Mesh*>(model);
	SubMesh *s;
	vector<SubMesh* >::iterator subMeshIterator;
	
	//pushMatrix();
	//ToDo: Per submesh transforms!!!!!!!!!!!!!!!!!!! - Ionut
	//glMultMatrixf(tempModel->getTransform()->getMatrix());
	//glMultMatrixf(tempModel->getParentMatrix());

	for(subMeshIterator = tempModel->getSubMeshes().begin(); 
		subMeshIterator != tempModel->getSubMeshes().end(); 
		++subMeshIterator)	{

		s = (*subMeshIterator);

		setMaterial(s->getMaterial());
		
		Shader* shader = model->getMaterial()->getShader();
		Texture2D* baseTexture = model->getMaterial()->getTexture(Material::TEXTURE_BASE);
		Texture2D* bumpTexture = model->getMaterial()->getTexture(Material::TEXTURE_BUMP);
		Texture2D* secondTexture = model->getMaterial()->getTexture(Material::TEXTURE_SECOND);

		if(baseTexture) baseTexture->Bind(0);
		if(bumpTexture) bumpTexture->Bind(1);
		if(secondTexture) secondTexture->Bind(2);

		if(shader){
			shader->bind();
				if(!GFXDevice::getInstance().getDeferredShading()){
     				shader->Uniform("enable_shadow_mapping", 0);
					shader->Uniform("tile_factor", 1.0f);
					shader->Uniform("textureCount",secondTexture != NULL ? 1 : 2);
					shader->UniformTexture("texDiffuse0",0);
					if(bumpTexture){
						shader->Uniform("mode", 1);
						shader->UniformTexture("texBump",1);
					}else{
						shader->Uniform("mode", 0);
					}
					if(secondTexture) shader->UniformTexture("texDiffuse1",2);

				}else{
					//ToDo: deffered rendering supports only one texture for now! -Ionut
					shader->Uniform("texDiffuse0",0);
				}
		}

		s->getGeometryVBO()->Enable();
		//	glDrawElements(GL_TRIANGLES, s->getIndices().size(), GL_UNSIGNED_INT, &(s->getIndices()[0]));
		s->getGeometryVBO()->Disable();

		if(shader) shader->unbind();
		if(secondTexture) secondTexture->Unbind(2);
		if(bumpTexture) bumpTexture->Unbind(1);
		if(baseTexture) baseTexture->Unbind(0);
		
	}
	//popMatrix();
}

void DX_API::renderElements(Type t, U32 count, const void* first_element,bool inverty)
{
}

void DX_API::setMaterial(Material* mat)
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

void DX_API::toggleWireframe(bool state)
{
}