#include "Headers/SceneNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/RenderStateBlock.h"

SceneNode::SceneNode(SCENE_NODE_TYPE type) : Resource(),
											 _material(NULL),
											 _drawState(true),
											 _noDefaultMaterial(false),
											 _exclusionMask(0),
											 _selected(false),
											 _shadowStateBlock(NULL),
											 _type(type)
{}

SceneNode::SceneNode(std::string name, SCENE_NODE_TYPE type) : Resource(name),
															   _material(NULL),
															   _drawState(true),
															   _noDefaultMaterial(false),
															   _exclusionMask(0),
															   _selected(false),
															   _shadowStateBlock(NULL),
															   _type(type)
{}

SceneNode::~SceneNode() {
	SAFE_DELETE(_shadowStateBlock);
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

void SceneNode::postDraw(){
	//Nothing yet
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
			_material = CreateResource<Material>(defaultMat);
		}
	}
	return _material;
}

void SceneNode::setMaterial(Material* m){
	if(m){ //If we need to update the material
		if(_material){ //If we had an old material
			if(_material->getMaterialId().i != m->getMaterialId().i){ //if the old material isn't the same as the new one
				PRINT_FN("Replacing material [ %s ] with material  [ %s ]",_material->getName().c_str(),m->getName().c_str());
				RemoveResource(_material);			//remove the old material
			}
		}
		_material = m;				   //set the new material
	}else{ //if we receive a null material, the we need to remove this node's material
		if(_material){
			PRINT_FN("Removing material [ %s ]",_material->getName().c_str());
			RemoveResource(_material);
		}
	}
}

void SceneNode::clearMaterials(){
	setMaterial(NULL);
}

void SceneNode::prepareMaterial(SceneGraphNode* const sgn){
	if(!_material/* || !sgn*/) return;
	if(GFX_DEVICE.getRenderStage() == REFLECTION_STAGE){
		SET_STATE_BLOCK(_material->getRenderState(REFLECTION_STAGE));
	}else{
		SET_STATE_BLOCK(_material->getRenderState(FINAL_STAGE));
	}
	GFX_DEVICE.setMaterial(_material);

	ShaderProgram* s = _material->getShaderProgram();
	Scene* activeScene = SceneManager::getInstance().getActiveScene();

	Texture2D* baseTexture = _material->getTexture(Material::TEXTURE_BASE);
	Texture2D* bumpTexture = _material->getTexture(Material::TEXTURE_BUMP);
	Texture2D* secondTexture = _material->getTexture(Material::TEXTURE_SECOND);
	Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
	Texture2D* specularMap = _material->getTexture(Material::TEXTURE_SPECULAR);

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
	

	s->bind();
	//The idea here is simple. Stop using glMultMatrixf for transforms
	//Move transformation calculations to shader. If no transform matrix is available,
	//revert to a legacy mode or legacy shader. 
	//ToDo: implement and test this! -Ionut

	/*Transform* t = sgn->getTransform();
	if(t){
		s->Uniform("transformMatrix",t->getMatrix());
		s->Uniform("parentTransformMatrix",t->getParentMatrix());
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
	if(LightManager::getInstance().shadowMappingEnabled()){
		s->Uniform("enable_shadow_mapping",_material->getReceivesShadows());
	}else{
		s->Uniform("enable_shadow_mapping",0);
	}
	s->Uniform("windDirectionX", activeScene->getWindDirX());
	s->Uniform("windDirectionZ", activeScene->getWindDirZ());
	s->Uniform("windSpeed", activeScene->getWindSpeed());
}

void SceneNode::releaseMaterial(){
	if(!_material) return;

	Texture2D* baseTexture = _material->getTexture(Material::TEXTURE_BASE);
	Texture2D* bumpTexture = _material->getTexture(Material::TEXTURE_BUMP);
	Texture2D* secondTexture = _material->getTexture(Material::TEXTURE_SECOND);
	Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
	Texture2D* specularMap = _material->getTexture(Material::TEXTURE_SPECULAR);
	
	if(specularMap) specularMap->Unbind(4);
	if(opacityMap) opacityMap->Unbind(3);
	if(bumpTexture) bumpTexture->Unbind(2);
	if(secondTexture) secondTexture->Unbind(1);
	if(baseTexture) baseTexture->Unbind(0);
}

void SceneNode::prepareShadowMaterial(SceneGraphNode* const sgn){
	if(getType() != TYPE_OBJECT3D) return;
	/// general shadow descriptor for objects without material
	if(!_material) { 
		if(!_shadowStateBlock){
			RenderStateBlockDescriptor shadowDesc;
			/// Cull back faces for shadow rendering
			shadowDesc.setCullMode(CULL_MODE_CCW);
			shadowDesc._fixedLighting = false;
			//shadowDesc._zBias = -0.0002f;
			shadowDesc.setColorWrites(false,false,false,false);
			_shadowStateBlock = GFX_DEVICE.createStateBlock(shadowDesc);
		}
		SET_STATE_BLOCK(_shadowStateBlock);
	}else{
		SET_STATE_BLOCK(_material->getRenderState(SHADOW_STAGE));
		GFX_DEVICE.setMaterial(_material);
	}
}

void SceneNode::releaseShadowMaterial(){

}

bool SceneNode::computeBoundingBox(SceneGraphNode* const sgn) {
	BoundingBox& bb = sgn->getBoundingBox();
	sgn->setInitialBoundingBox(bb);
	bb.isComputed() = true;
	return true;
}

bool SceneNode::unload(){
	clearMaterials();

	return true;
}

void SceneNode::drawBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	///This is the only immediate mode draw call left in the rendering api's
	GFX_DEVICE.drawBox3D(bb.getMin(),bb.getMax());
}

void SceneNode::removeFromDrawExclusionMask(U8 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask &= ~stageMask;
}

void SceneNode::addToDrawExclusionMask(U8 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask |= static_cast<RENDER_STAGE>(stageMask);
}

bool SceneNode::getDrawState(RENDER_STAGE currentStage)  const {
	return (_exclusionMask & currentStage) == currentStage ? false : true;
}
