#include "SceneNode.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/CameraManager.h"
#include "Rendering/Frustum.h"

SceneGraphNode* SceneNode::getSceneGraphNode(){
	return SceneManager::getInstance().getActiveScene()->getSceneGraph()->findNode(getName());
}

void SceneNode::removeCopy(){
  decRefCount();
  Material* mat = getMaterial();
  if(mat){
	mat->removeCopy();
  }
}

void SceneNode::createCopy(){
  incRefCount();
  Material* mat = getMaterial();
  if(mat){
	mat->createCopy();
  }
}

void SceneNode::onDraw(){
	Material* mat = getMaterial();
	if(mat){
		mat->computeLightShaders();
	}
}

bool SceneNode::isInView(bool distanceCheck,BoundingBox& boundingBox){
	Frustum& frust = Frustum::getInstance();
	const vec3& eye_pos = CameraManager::getInstance().getActiveCamera()->getEye();
	
	vec3& center = boundingBox.getCenter();
	F32   halfExtent = boundingBox.getHalfExtent().length();
	if(distanceCheck){
		
		vec3 vEyeToChunk = center - eye_pos;

		if((vEyeToChunk.length() + halfExtent) > SceneManager::getInstance().getActiveScene()->getGeneralVisibility()){
			return false;
		}
	}

	if(!boundingBox.ContainsPoint(eye_pos))
	{
		F32 radius = (boundingBox.getMax()-center).length();
		switch(frust.ContainsSphere(center, radius)) {
				case FRUSTUM_OUT: 	
					return false;
				
				case FRUSTUM_INTERSECT:	
					if(frust.ContainsBoundingBox(boundingBox) == FRUSTUM_OUT)
						return false;
			}
	}

	return true;
}


	
	
Material* SceneNode::getMaterial(){
	if(_material == NULL){
		if(!_noDefaultMaterial){
			ResourceDescriptor defaultMat("defaultMaterial");
			_material = ResourceManager::getInstance().loadResource<Material>(defaultMat);
		}
	}
	return _material;
}


void SceneNode::setMaterial(Material* m){
	if(_material != NULL) {
		RemoveResource(_material);
	}
	if(!m) return;
	_material = m;
}

void SceneNode::clearMaterials(){
	setMaterial(NULL);
}

void SceneNode::prepareMaterial(){
	GFXDevice& gfx = GFXDevice::getInstance();
	if(!_material) return;
	if(gfx.getDepthMapRendering()) return;

	Shader* s = _material->getShader();
	Transform* t = getSceneGraphNode()->getTransform();
	Scene* activeScene = SceneManager::getInstance().getActiveScene();

	gfx.setMaterial(_material);
	Texture2D* baseTexture = NULL;
	Texture2D* bumpTexture = NULL;
	Texture2D* secondTexture = NULL;
	if(gfx.getActiveRenderState().texturesEnabled()){
		baseTexture = _material->getTexture(Material::TEXTURE_BASE);
		bumpTexture = _material->getTexture(Material::TEXTURE_BUMP);
		secondTexture = _material->getTexture(Material::TEXTURE_SECOND);
	}

	U8 count = 0;
	if(baseTexture){
		baseTexture->Bind(0);
		count++;
	}
	if(secondTexture){
		secondTexture->Bind(1);
		count++;
	}
	if(bumpTexture){
		bumpTexture->Bind(2);
	}
	if(s){
		s->bind();
		if(baseTexture){
			s->UniformTexture("texDiffuse0",0);
		}
		if(secondTexture){
			s->UniformTexture("texDiffuse1",1);
		}
		if(bumpTexture){
			s->UniformTexture("texBump",2);
		}
		if(t){
			s->Uniform("scale",t->getScale());
			s->Uniform("transformMatrix",t->getMatrix());
			s->Uniform("parentTransformMatrix",t->getParentMatrix());
		}else{
			s->Uniform("scale",vec3(1,1,1)); //ToDo: fix this! -Ionut
		}
		
		s->Uniform("material",_material->getMaterialMatrix());
		s->Uniform("modelViewInvMatrix",Frustum::getInstance().getModelviewInvMatrix());
		s->Uniform("lightProjectionMatrix",gfx.getLightProjectionMatrix());
		s->Uniform("textureCount",count);
		s->Uniform("depth_map_size",1024);
		s->Uniform("parallax_factor", 0.02f);
		s->Uniform("relief_factor", 0.08f);
		s->Uniform("tile_factor", 1.0f);
		s->Uniform("time", GETTIME());
		s->Uniform("windDirectionX", activeScene->getWindDirX());
		s->Uniform("windDirectionZ", activeScene->getWindDirZ());
		s->Uniform("windSpeed", activeScene->getWindSpeed());
		s->Uniform("enable_shadow_mapping", (activeScene->getDepthMap(0) != NULL ||  activeScene->getDepthMap(1) != NULL) ? 1 : 0);
		s->Uniform("mode", bumpTexture != NULL ? 1 : 0);
	}
}

void SceneNode::releaseMaterial(){
	if(!_material) return;
	if(GFXDevice::getInstance().getDepthMapRendering()) return;
	Texture2D* baseTexture = NULL;
	Texture2D* bumpTexture = NULL;
	Texture2D* secondTexture = NULL;
	if(GFXDevice::getInstance().getActiveRenderState().texturesEnabled()){
		baseTexture = _material->getTexture(Material::TEXTURE_BASE);
		bumpTexture = _material->getTexture(Material::TEXTURE_BUMP);
		secondTexture = _material->getTexture(Material::TEXTURE_SECOND);
	}
	if(bumpTexture) bumpTexture->Unbind(2);
	if(secondTexture) secondTexture->Unbind(1);
	if(baseTexture) baseTexture->Unbind(0);
	if(_material->getShader()){
		_material->getShader()->unbind();
	}
}

bool SceneNode::computeBoundingBox(SceneGraphNode* node) {
	BoundingBox& bb = node->getBoundingBox();
	node->setInitialBoundingBox(bb);
	bb.isComputed() = true;
	return true;
}

bool SceneNode::unload(){
	clearMaterials();

	return true;
}