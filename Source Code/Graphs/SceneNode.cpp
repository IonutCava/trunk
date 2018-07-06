#include "Headers/SceneNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Mesh.h" 
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Managers/Headers/ShaderManager.h"

SceneNode::SceneNode(SCENE_NODE_TYPE type) : Resource(),
											 _material(NULL),
											 _drawState(true),
											 _noDefaultMaterial(false),
											 _exclusionMask(0),
											 _selected(false),
											 _shadowStateBlock(NULL),
											 _type(type)
{
}

SceneNode::SceneNode(std::string name, SCENE_NODE_TYPE type) : Resource(name),
															   _material(NULL),
															   _drawState(true),
															   _noDefaultMaterial(false),
															   _exclusionMask(0),
															   _selected(false),
															   _shadowStateBlock(NULL),
															   _type(type)
{
}

SceneNode::~SceneNode() {
	SAFE_DELETE(_shadowStateBlock);
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
	Scene* activeScene = GET_ACTIVE_SCENE();
	vec3<F32> center = boundingBox.getCenter();
	F32 radius = (boundingBox.getMax()-center).length();	
	F32 halfExtent = boundingBox.getHalfExtent().length();

	if(distanceCheck){
		vec3<F32> vEyeToChunk = center - frust.getEyePos();
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
			REGISTER_TRACKED_DEPENDENCY(_material);
		}
	}
	return _material;
}

void SceneNode::setMaterial(Material* m){
	if(m){ //If we need to update the material
		if(_material){ //If we had an old material
			if(_material->getMaterialId().i != m->getMaterialId().i){ //if the old material isn't the same as the new one
				PRINT_FN(Locale::get("REPLACE_MATERIAL"),_material->getName().c_str(),m->getName().c_str());
				RemoveResource(_material);			//remove the old material
				UNREGISTER_TRACKED_DEPENDENCY(_material);
			}
		}
		_material = m;				   //set the new material
		REGISTER_TRACKED_DEPENDENCY(_material);
	}else{ //if we receive a null material, the we need to remove this node's material
		if(_material){
			UNREGISTER_TRACKED_DEPENDENCY(_material);
			RemoveResource(_material);
		}
	}
}

void SceneNode::clearMaterials(){
	setMaterial(NULL);
}

void SceneNode::prepareMaterial(SceneGraphNode const* const sgn){
	if(!_material/* || !sgn*/) return;
	if(GFX_DEVICE.getRenderStage() == REFLECTION_STAGE){
		SET_STATE_BLOCK(_material->getRenderState(REFLECTION_STAGE));
	}else{
		SET_STATE_BLOCK(_material->getRenderState(FINAL_STAGE));
	}
	GFX_DEVICE.setMaterial(_material);

	ShaderProgram* s = _material->getShaderProgram();
	Scene* activeScene = GET_ACTIVE_SCENE();

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

	s->Uniform("hasAnimations",false);
	s->Uniform("shadowPass", false);
	/// For 3D objects
	if(getType() == TYPE_OBJECT3D && ParamHandler::getInstance().getParam<bool>("mesh.playAnimations")){
		Object3D* obj = dynamic_cast<Object3D* >(this);
		/// For Mesh objects
		if(obj->getType() == SUBMESH){
			/// Apply bone transforms
			std::vector<mat4<F32> >& transforms = dynamic_cast<SubMesh* >(obj)->GetTransforms();
			if(!transforms.empty()){
				s->Uniform("hasAnimations",	true);	
				s->Uniform("boneTransforms", transforms);
			}
		}
	}
	
	if(baseTexture)   {
		s->UniformTexture("texDiffuse0",0);
		s->Uniform("texDiffuse0Op", (I32)_material->getTextureOperation(Material::TEXTURE_BASE));
	}
	if(secondTexture) {
		s->UniformTexture("texDiffuse1",1);
		s->Uniform("texDiffuse1Op", (I32)_material->getTextureOperation(Material::TEXTURE_SECOND));
	}
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
	s->Uniform("opacity", _material->getOpacityValue());
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
		ShaderProgram* s = _material->getShaderProgram();

		if(s){
			Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);

			s->bind();
			s->Uniform("mode", 0);
			s->Uniform("shadowPass", true);
		
			if(opacityMap){
				opacityMap->Bind(3);
				s->UniformTexture("opacityMap",3);
				s->Uniform("hasOpacity", true);
			}else{
				s->Uniform("hasOpacity", false);
			}
			/// For 3D objects: Animate them so shadows are also updated
			if(getType() == TYPE_OBJECT3D){
				Object3D* obj = dynamic_cast<Object3D* >(this);
				/// For Mesh objects
				if(obj->getType() == SUBMESH){
					/// Apply bone transforms
					std::vector<mat4<F32> >& transforms = dynamic_cast<SubMesh* >(obj)->GetTransforms();
					if(transforms.empty()){
						s->Uniform("hasAnimations",	false);	
					}else{
						///If we have transforms, use animations
						s->Uniform("hasAnimations",	true);	
						s->Uniform("boneTransforms", transforms);
					}
				}
			}
		}
	}
}


void SceneNode::releaseShadowMaterial(){
	if(!_material) return;
	Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
	if(opacityMap) opacityMap->Unbind(3);
}

bool SceneNode::computeBoundingBox(SceneGraphNode* const sgn) {
	BoundingBox& bb = sgn->getBoundingBox();
	sgn->setInitialBoundingBox(bb);
	bb.isComputed() = true;
	return true;
}

void SceneNode::updateBBatCurrentFrame(SceneGraphNode* const sgn){
	sgn->updateBB(false);
}

void SceneNode::updateTransform(SceneGraphNode* const sgn) {
}

bool SceneNode::unload(){
	clearMaterials();

	return true;
}

void SceneNode::drawBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	mat4<F32> boundingBoxTransformMatrix;
	Transform* tempTransform = sgn->getTransform();
	if(tempTransform){
		boundingBoxTransformMatrix = tempTransform->getGlobalMatrix();
	}
	///This is the only immediate mode draw call left in the rendering api's
	GFX_DEVICE.drawBox3D(bb.getMin(),bb.getMax(),boundingBoxTransformMatrix);
}

void SceneNode::removeFromDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask &= ~stageMask;
}

void SceneNode::addToDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask |= static_cast<RENDER_STAGE>(stageMask);
}

bool SceneNode::getDrawState(RENDER_STAGE currentStage)  const {
	return (_exclusionMask & currentStage) == currentStage ? false : true;
}

void SceneNode::sceneUpdate(D32 sceneTime){
}