#include "Headers/SceneNode.h"

#include "Rendering/Headers/Frustum.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"

SceneNode::SceneNode(const SceneNodeType& type) : Resource(),
										   _material(NULL),
                                           _customShader(NULL),
										   _selected(false),
										   _refreshMaterialData(true),
										   _type(type),
										   _lodLevel(0),
										   _LODcount(1), ///<Defaults to 1 LOD level
										   _physicsAsset(NULL)
{
}

SceneNode::SceneNode(const std::string& name,const SceneNodeType& type) : Resource(name),
															 _material(NULL),
                                                             _customShader(NULL),
															 _selected(false),
															 _refreshMaterialData(true),
															 _type(type),
															 _lodLevel(0),
															 _LODcount(1), ///<Defaults to 1 LOD level
															 _physicsAsset(NULL)
{
}

SceneNode::~SceneNode() {
	SAFE_DELETE(_physicsAsset);
}

void SceneNode::sceneUpdate(const U32 sceneTime,SceneGraphNode* const sgn){
	if(!_material) return;
	_refreshMaterialData = _material->isDirty();
	_material->clean();
}

void SceneNode::onDraw(const RenderStage& currentStage){
	Material* mat = getMaterial();
	if(mat){
		mat->computeShader(false);
		mat->computeShader(false,DEPTH_STAGE);
	}
}

void SceneNode::preFrameDrawEnd(SceneGraphNode* const sgn){
	//draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifacts
	BoundingBox& bb = sgn->getBoundingBox();
	//Draw the bounding box if it's always on or if the scene demands it
	if(bb.getVisibility() || GET_ACTIVE_SCENE()->renderState()->drawBBox()){
		drawBoundingBox(sgn);
	}
}

bool SceneNode::isInView(const bool distanceCheck,const BoundingBox& boundingBox,const BoundingSphere& sphere){
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

    const vec3<F32>& eye = frust.getEyePos();
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
    //UpgradableReadLock ur_lock(_materialLock);
	if(_material == NULL){
		if(!_renderState._noDefaultMaterial){
			ResourceDescriptor defaultMat("defaultMaterial");
            //UpgradeToWriteLock uw_lock(ur_lock);
			_material = CreateResource<Material>(defaultMat);
			REGISTER_TRACKED_DEPENDENCY(_material);
		}
	}
	return _material;
}

void SceneNode::setMaterial(Material* const m){
	if(m){ //If we need to update the material
        //UpgradableReadLock ur_lock(_materialLock);
		if(_material){ //If we had an old material
			if(_material->getMaterialId().i != m->getMaterialId().i){ //if the old material isn't the same as the new one
				PRINT_FN(Locale::get("REPLACE_MATERIAL"),_material->getName().c_str(),m->getName().c_str());
                //UpgradeToWriteLock uw_lock(ur_lock);
				RemoveResource(_material);			//remove the old material
				UNREGISTER_TRACKED_DEPENDENCY(_material);
                //ur_lock.lock();
			}
		}
        //UpgradeToWriteLock uw_lock(ur_lock);
		_material = m;				   //set the new material
		REGISTER_TRACKED_DEPENDENCY(_material);
	}else{ //if we receive a null material, the we need to remove this node's material
        //UpgradableReadLock ur_lock(_materialLock);
		if(_material){
            //UpgradeToWriteLock uw_lock(ur_lock);
			UNREGISTER_TRACKED_DEPENDENCY(_material);
			RemoveResource(_material);
		}
	}
}

void SceneNode::clearMaterials(){
	setMaterial(NULL);
}

void SceneNode::prepareMaterial(SceneGraphNode* const sgn){
    //UpgradableReadLock ur_lock(_materialLock);
    if(!_material)  return;

	if(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE)){
		SET_STATE_BLOCK(_material->getRenderState(REFLECTION_STAGE));
	}else{
		SET_STATE_BLOCK(_material->getRenderState(FINAL_STAGE));
	}

	ShaderProgram* s = _material->getShaderProgram();
	Scene* activeScene = GET_ACTIVE_SCENE();

	Texture2D* baseTexture = _material->getTexture(Material::TEXTURE_BASE);
	Texture2D* bumpTexture = _material->getTexture(Material::TEXTURE_BUMP);
	Texture2D* secondTexture = _material->getTexture(Material::TEXTURE_SECOND);
	Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
	Texture2D* specularMap = _material->getTexture(Material::TEXTURE_SPECULAR);
	if(baseTexture)   baseTexture->Bind(Material::FIRST_TEXTURE_UNIT);
	if(secondTexture) secondTexture->Bind(Material::SECOND_TEXTURE_UNIT);
	if(bumpTexture)   bumpTexture->Bind(Material::BUMP_TEXTURE_UNIT);
	if(opacityMap) 	  opacityMap->Bind(Material::OPACITY_TEXTURE_UNIT);
	if(specularMap)   specularMap->Bind(Material::SPECULAR_TEXTURE_UNIT);
    
	s->bind();

	if(baseTexture)   s->Uniform("texDiffuse0Op", (I32)_material->getTextureOperation(Material::TEXTURE_BASE));
	if(secondTexture) s->Uniform("texDiffuse1Op", (I32)_material->getTextureOperation(Material::TEXTURE_SECOND));
		
	s->Uniform("material",_material->getMaterialMatrix());
	s->Uniform("opacity", _material->getOpacityValue());
	s->Uniform("textureCount",_material->getTextureCount());
	if(LightManager::getInstance().shadowMappingEnabled()){
		s->Uniform("dvd_enableShadowMapping",_material->getReceivesShadows());
	}else{
		s->Uniform("dvd_enableShadowMapping",false);
	}
	
	if(LightManager::getInstance().shadowMappingEnabled()){
        s->Uniform("worldHalfExtent", GET_ACTIVE_SCENE()->getSceneGraph()->getRoot()->getBoundingBox().getWidth() * 0.5f);
		s->Uniform("dvd_lightProjectionMatrices",LightManager::getInstance().getLightProjectionMatricesCache());
	}

    const vectorImpl<I32>& types = LightManager::getInstance().getLightTypesForCurrentNode();
    const vectorImpl<I32>& enabled = LightManager::getInstance().getLightsEnabledForCurrentNode();
	const vectorImpl<I32>& lightShadowCast = LightManager::getInstance().getShadowCastingLightsForCurrentNode();
	s->Uniform("dvd_lightCount", LightManager::getInstance().getLightCountForCurrentNode());
	s->Uniform("dvd_lightType",types);
    s->Uniform("dvd_lightEnabled",enabled);
	s->Uniform("dvd_lightCastsShadows",lightShadowCast);
	s->Uniform("windDirection",vec2<F32>(activeScene->state()->getWindDirX(),activeScene->state()->getWindDirZ()));
	s->Uniform("windSpeed", activeScene->state()->getWindSpeed());

	if(!sgn->animationTransforms().empty()){
		s->Uniform("hasAnimations", true);
		s->Uniform("boneTransforms", sgn->animationTransforms());
	}else{
		s->Uniform("hasAnimations",false);
	}
}

void SceneNode::releaseMaterial(){
    //UpgradableReadLock ur_lock(_materialLock);
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

void SceneNode::prepareDepthMaterial(SceneGraphNode* const sgn){
	if(getType() != TYPE_OBJECT3D && getType() != TYPE_TERRAIN) return;
    //UpgradableReadLock ur_lock(_materialLock);
	// general depth descriptor for objects without material
	if(!_material) {
		SET_STATE_BLOCK(_renderState.getDepthStateBlock());
		return;
	}

	SET_STATE_BLOCK(_material->getRenderState(DEPTH_STAGE));

	ShaderProgram* s = _material->getShaderProgram(DEPTH_STAGE);
	assert(s != NULL);
	s->bind();

	Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);

	if(opacityMap)	opacityMap->Bind(Material::OPACITY_TEXTURE_UNIT );
	
	s->Uniform("opacity", _material->getOpacityValue());

	if(!sgn->animationTransforms().empty()){
		s->Uniform("hasAnimations", true);
		s->Uniform("boneTransforms", sgn->animationTransforms());
	}else{
		s->Uniform("hasAnimations",false);
	}
}

void SceneNode::releaseDepthMaterial(){
    //UpgradableReadLock ur_lock(_materialLock);
	if(!_material) return;
	Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
	if(opacityMap) opacityMap->Unbind(0);
}

bool SceneNode::computeBoundingBox(SceneGraphNode* const sgn) {
	BoundingBox& bb = sgn->getBoundingBox();
	sgn->setInitialBoundingBox(bb);
	bb.setComputed(true);
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
	/*Transform* tempTransform = sgn->getTransform();
	if(tempTransform){
		boundingBoxTransformMatrix = tempTransform->getGlobalMatrix();
	}*/

	GFX_DEVICE.drawBox3D(bb.getMin(),bb.getMax(),boundingBoxTransformMatrix);
}