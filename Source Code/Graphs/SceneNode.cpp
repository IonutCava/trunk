#include "Headers/SceneNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/Headers/Frustum.h"

SceneNode::SceneNode() : Resource(),
						 _material(NULL),
						 _renderState(true),
						 _noDefaultMaterial(false),
						 _sortKey(0),
						 _exclusionMask(0),
						 _selected(false)
{}

SceneNode::SceneNode(std::string name) : Resource(name),
								        _material(NULL),
										_renderState(true),
										_noDefaultMaterial(false),
										_sortKey(0),
										_exclusionMask(0),
										_selected(false)
{}

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
	Scene* activeScene = SceneManager::getInstance().getActiveScene();
	vec3 center = boundingBox.getCenter();
	F32 radius = (boundingBox.getMax()-center).length();	
	F32 halfExtent = boundingBox.getHalfExtent().length();

	if(distanceCheck){
		vec3 vEyeToChunk = center - frust.getEyePos();
		if((vEyeToChunk.length() + halfExtent) > activeScene->getGeneralVisibility()){
			return false;
		}
	}

	if(!boundingBox.ContainsPoint(frust.getEyePos())){
		I8 resSphereInFrustum = frust.ContainsSphere(center, radius);
		switch(resSphereInFrustum) {
			case FRUSTUM_OUT: return false;
			case FRUSTUM_INTERSECT:	{
				I8 resBoxInFrustum = frust.ContainsBoundingBox(boundingBox);
				switch(resBoxInFrustum) {
					case FRUSTUM_OUT: return false;
				}
			}
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
	if(m){ //If we need to update the material
		if(_material){ //If we had an old material
			if(_material->getMaterialId() != m->getMaterialId()){ //if the old material isn't the same as the new one
				Console::getInstance().printfn("Replacing material [ %s ] with material  [ %s ]",_material->getName().c_str(),m->getName().c_str());
				RemoveResource(_material);			//remove the old material
			}
		}
		_material = m;						//set the new material
		_sortKey = m->getMaterialId(); //and uptade the sort key
	}else{ //if we receive a null material, the we need to remove this node's material
		if(_material){
			Console::getInstance().printfn("Removing material [ %s ]",_material->getName().c_str());
			RemoveResource(_material);
		}
		_sortKey = 0;
	}
}

void SceneNode::clearMaterials(){
	setMaterial(NULL);
}

void SceneNode::prepareMaterial(SceneGraphNode* const sgn){
	if(!_material/* || !sgn*/) return;

	GFXDevice& gfx = GFXDevice::getInstance();
	gfx.setRenderState(_material->getRenderState());
	gfx.setMaterial(_material);

	Shader* s = _material->getShader();
	Scene* activeScene = SceneManager::getInstance().getActiveScene();

	Texture2D* baseTexture = NULL;
	Texture2D* bumpTexture = NULL;
	Texture2D* secondTexture = NULL;
	Texture2D* opacityMap = NULL;
	Texture2D* specularMap = NULL;
	
	if(gfx.getActiveRenderState().texturesEnabled()){
		baseTexture = _material->getTexture(Material::TEXTURE_BASE);
		bumpTexture = _material->getTexture(Material::TEXTURE_BUMP);
		secondTexture = _material->getTexture(Material::TEXTURE_SECOND);
		opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
		specularMap = _material->getTexture(Material::TEXTURE_SPECULAR);
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
	if(bumpTexture)	bumpTexture->Bind(2);
	if(opacityMap)	opacityMap->Bind(3);
	if(specularMap) specularMap->Bind(4);
	
	if(s){
		s->bind();

		//The idea here is simple. Stop using glMultMatrixf for transforms
		//Move transformation calculations to shader. If no transform matrix is available,
		//revert to a legacy mode or legacy shader. 
		//ToDo: implement and test this! -Ionut

		/*Transform* t = sgn->getTransform();
		if(t){
			s->Uniform("transformMatrix",t->getMatrix());
			s->Uniform("parentTransformMatrix",t->getParentMatrix());
		}else{
			s->Uniform("legacyMode",true);
		}*/
		if(baseTexture)   s->UniformTexture("texDiffuse0",0);
		if(secondTexture) s->UniformTexture("texDiffuse1",1);
		if(bumpTexture){
			s->UniformTexture("texBump",2);
			s->Uniform("mode", 1);
		}else{
			s->Uniform("mode",0);
		}
		if(opacityMap){
			s->UniformTexture("opacityMap",3);
			s->Uniform("hasOpacity", true);
		}else{
			s->Uniform("hasOpacity", false);
		}
		if(specularMap){
			s->UniformTexture("texSpecular",4);
			s->Uniform("hasSpecular",true);
		}else{
			s->Uniform("hasSpecular",false);
		}
		s->Uniform("material",_material->getMaterialMatrix());
		s->Uniform("textureCount",count);
		s->Uniform("parallax_factor", 1.f);
		s->Uniform("relief_factor", 1.f);
		s->Uniform("tile_factor", 1.0f);
		if(LightManager::getInstance().shadowMappingEnabled()){
			s->Uniform("enable_shadow_mapping",_material->getReceivesShadows());
		}else{
			s->Uniform("enable_shadow_mapping",0);
		}
		s->Uniform("windDirectionX", activeScene->getWindDirX());
		s->Uniform("windDirectionZ", activeScene->getWindDirZ());
		s->Uniform("windSpeed", activeScene->getWindSpeed());
	}
}

void SceneNode::releaseMaterial(){
	if(!_material) return;
	GFXDevice& gfx = GFXDevice::getInstance();
	gfx.restoreRenderState();

	Texture2D* baseTexture = NULL;
	Texture2D* bumpTexture = NULL;
	Texture2D* secondTexture = NULL;
	Texture2D* opacityMap = NULL;
	Texture2D* specularMap = NULL;
	if(gfx.getActiveRenderState().texturesEnabled() ){
		baseTexture = _material->getTexture(Material::TEXTURE_BASE);
		bumpTexture = _material->getTexture(Material::TEXTURE_BUMP);
		secondTexture = _material->getTexture(Material::TEXTURE_SECOND);
		opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
		specularMap = _material->getTexture(Material::TEXTURE_SPECULAR);
	}
	if(specularMap) specularMap->Unbind(4);
	if(opacityMap) opacityMap->Unbind(3);
	if(bumpTexture) bumpTexture->Unbind(2);
	if(secondTexture) secondTexture->Unbind(1);
	if(baseTexture) baseTexture->Unbind(0);

	if(_material->getShader()){
		_material->getShader()->unbind();
	}
}

bool SceneNode::computeBoundingBox(SceneGraphNode* const node) {
	BoundingBox& bb = node->getBoundingBox();
	node->setInitialBoundingBox(bb);
	bb.isComputed() = true;
	return true;
}

bool SceneNode::unload(){
	clearMaterials();

	return true;
}

void SceneNode::removeFromRenderExclusionMask(U8 stageMask) {
	assert(stageMask & ~(INVALID_STAGE-1) == 0);
	_exclusionMask &= ~stageMask;
}

void SceneNode::addToRenderExclusionMask(U8 stageMask) {
	assert(stageMask & ~(INVALID_STAGE-1) == 0);
	_exclusionMask |= static_cast<RENDER_STAGE>(stageMask);
}

bool SceneNode::getRenderState(RENDER_STAGE currentStage)  const {
	return (_exclusionMask & currentStage) == currentStage ? false : true;
}
