#include "Headers/SceneNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Frustum.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Mesh.h" 
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Managers/Headers/ShaderManager.h"

SceneNode::SceneNode(SceneNodeType type) : Resource(),
										   _material(NULL),
										   _selected(false),
										   _type(type),
										   _lodLevel(0),
										   _LODcount(1), ///<Defaults to 1 LOD level
										   _physicsAsset(NULL)
{
}

SceneNode::SceneNode(std::string name, SceneNodeType type) : Resource(name),
															 _material(NULL),
															 _selected(false),
															 _type(type),
															 _lodLevel(0),
															 _LODcount(1), ///<Defaults to 1 LOD level
															 _physicsAsset(NULL)
{
}

SceneNode::~SceneNode() {
	SAFE_DELETE(_physicsAsset);
}

void SceneNode::onDraw(){
	Material* mat = getMaterial();
	if(mat){
		mat->computeShader(false);
		mat->computeShader(false,SHADOW_STAGE);
	}
}
void SceneNode::preFrameDrawEnd(SceneGraphNode* const sgn){
	///draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifcats
	BoundingBox& bb = sgn->getBoundingBox();
	///Draw the bounding box if it's always on or if the scene demands it
	if(bb.getVisibility() || GET_ACTIVE_SCENE()->renderState()->drawBBox()){
		drawBoundingBox(sgn);
	}
}

bool SceneNode::isInView(bool distanceCheck,BoundingBox& boundingBox,const BoundingSphere& sphere){

	Frustum& frust = Frustum::getInstance();
	Scene* activeScene = GET_ACTIVE_SCENE();
	vec3<F32> center = sphere.getCenter();
	F32 radius = sphere.getRadius();	
	F32 halfExtent = boundingBox.getHalfExtent().length();
	vec3<F32> eyeToNode    = center - frust.getEyePos();
	F32       cameraDistance = eyeToNode.length();

	if(distanceCheck){
		if((cameraDistance + halfExtent) > activeScene->state()->getGeneralVisibility()){
			return false;
		}
	}

	U8 lod = 0;
	if(cameraDistance > SCENE_NODE_LOD0)		lod = 2;
	else if(cameraDistance > SCENE_NODE_LOD1)	lod = 1;
	_lodLevel = lod;

    vec3<F32>& eye = frust.getEyePos();
	if(!boundingBox.ContainsPoint(eye)){
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
		if(!_renderState._noDefaultMaterial){
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

void SceneNode::prepareMaterial(SceneGraphNode* const sgn){
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
	
	if(baseTexture)   {
		s->UniformTexture("texDiffuse0",0);
		s->Uniform("texDiffuse0Op", (I32)_material->getTextureOperation(Material::TEXTURE_BASE));
	}
	if(secondTexture) {
		s->UniformTexture("texDiffuse1",1);
		s->Uniform("texDiffuse1Op", (I32)_material->getTextureOperation(Material::TEXTURE_SECOND));
	}

	if(bumpTexture && _material->getBumpMethod() != Material::BUMP_NONE){
		s->UniformTexture("texBump",2);
		s->Uniform("parallax_factor", 1.f);
		s->Uniform("relief_factor", 1.f);
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
	s->Uniform("bumpMapLightId",0);
	if(LightManager::getInstance().shadowMappingEnabled()){
		s->Uniform("enable_shadow_mapping",_material->getReceivesShadows());
		s->Uniform("lightProjectionMatrices",LightManager::getInstance().getLightProjectionMatricesCache());
		s->Uniform("lightBleedBias", 0.001f);
	}else{
		s->Uniform("enable_shadow_mapping",false);
	}
	s->Uniform("light_count", LightManager::getInstance().getLightCountForCurrentNode());
	vectorImpl<I32>& types = LightManager::getInstance().getLightTypesForCurrentNode();
	s->Uniform("lightType",types);
	s->Uniform("windDirection",vec2<F32>(activeScene->state()->getWindDirX(),activeScene->state()->getWindDirZ()));
	s->Uniform("windSpeed", activeScene->state()->getWindSpeed());
	setSpecialShaderConstants(s);
}

void SceneNode::setSpecialShaderConstants(ShaderProgram* const shader) {
	shader->Uniform("hasAnimations",false);
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
	if(getType() != TYPE_OBJECT3D && getType() != TYPE_TERRAIN) return;
	/// general shadow descriptor for objects without material
	if(!_material) { 
		SET_STATE_BLOCK(_renderState.getShadowStateBlock());

	}else{

		SET_STATE_BLOCK(_material->getRenderState(SHADOW_STAGE));
		GFX_DEVICE.setMaterial(_material);
		ShaderProgram* s = _material->getShaderProgram(SHADOW_STAGE);

		if(s){
			Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);

			s->bind();

			if(opacityMap){
				opacityMap->Bind(0);
				s->UniformTexture("opacityMap",0);
				s->Uniform("hasOpacity", true);
			}else{
				s->Uniform("hasOpacity", false);
			}
			s->Uniform("opacity", _material->getOpacityValue());
			setSpecialShaderConstants(s);
		}
	}
}


void SceneNode::releaseShadowMaterial(){
	if(!_material) return;
	Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
	if(opacityMap) opacityMap->Unbind(0);
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

	GFX_DEVICE.drawBox3D(bb.getMin(),bb.getMax(),boundingBoxTransformMatrix);
}