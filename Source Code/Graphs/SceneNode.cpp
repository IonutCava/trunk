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
                                           _refreshMaterialData(true),
                                           _type(type),
                                           _lodLevel(0),
                                           _LODcount(1), ///<Defaults to 1 LOD level
                                           _physicsAsset(NULL)
{
    U8 i = 0, j = 0;
    for(; i <  Material::TEXTURE_UNIT0; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation%d", Material::TEXTURE_UNIT0 + i);

    for(i = Material::TEXTURE_UNIT0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation%d", j++);
}

SceneNode::SceneNode(const std::string& name,const SceneNodeType& type) : Resource(name),
                                                             _material(NULL),
                                                             _customShader(NULL),
                                                             _refreshMaterialData(true),
                                                             _type(type),
                                                             _lodLevel(0),
                                                             _LODcount(1), ///<Defaults to 1 LOD level
                                                             _physicsAsset(NULL)
{
    U8 i = 0, j = 0;
    for(; i <  Material::TEXTURE_UNIT0; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation%d", Material::TEXTURE_UNIT0 + i);

    for(i = Material::TEXTURE_UNIT0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation%d", j++);
}

SceneNode::~SceneNode() {
    SAFE_DELETE(_physicsAsset);
}

void SceneNode::sceneUpdate(const D32 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    if(!_material)
        return;

    _refreshMaterialData = _material->isDirty();
    _material->clean();
}

void SceneNode::onDraw(const RenderStage& currentStage){
    Material* mat = getMaterial();

    if(!mat)
        return;

    mat->computeShader(false);
    mat->computeShader(false,DEPTH_STAGE);
}

void SceneNode::preFrameDrawEnd(SceneGraphNode* const sgn){
    //draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifacts
    //Draw the bounding box if it's always on or if the scene demands it
    if(sgn->getBoundingBox().getVisibility() || GET_ACTIVE_SCENE()->renderState().drawBBox()){
        drawBoundingBox(sgn);
    }
}

bool SceneNode::isInView(const BoundingBox& boundingBox,const BoundingSphere& sphere, const bool distanceCheck){
    Frustum& frust = Frustum::getInstance();

    const vec3<F32>& center  = sphere.getCenter();
    vec3<F32> eyeToNode      = center - frust.getEyePos();
    F32       cameraDistance = eyeToNode.length();

    if(distanceCheck &&
       cameraDistance + boundingBox.getHalfExtent().length() > GET_ACTIVE_SCENE()->state().getGeneralVisibility())
            return false;

    if(!boundingBox.ContainsPoint(frust.getEyePos())){
        switch(frust.ContainsSphere(center, sphere.getRadius())) {
            case FRUSTUM_OUT: return false;
            case FRUSTUM_INTERSECT:	{
                if(!frust.ContainsBoundingBox(boundingBox)) return false;
            }
        }
    }

    U8 lod = 0;
    if(cameraDistance > Config::SCENE_NODE_LOD0)		lod = 2;
    else if(cameraDistance > Config::SCENE_NODE_LOD1)	lod = 1;
    _lodLevel = lod;

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
    if(!_material)
        return;

    if(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE))
        SET_STATE_BLOCK(_material->getRenderState(REFLECTION_STAGE));
    else
        SET_STATE_BLOCK(_material->getRenderState(FINAL_STAGE));

    ShaderProgram* s = _material->getShaderProgram();
    Scene* activeScene = GET_ACTIVE_SCENE();
    LightManager& lightMgr = LightManager::getInstance();

    s->bind();

    Texture2D* texture = NULL;
    for(U16 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        if((texture = _material->getTexture(i)) != NULL){
            texture->Bind(i);

            if(i >= Material::TEXTURE_UNIT0)
                s->Uniform(_textureOperationUniformSlots[i], (I32)_material->getTextureOperation(i));
        }

    s->Uniform("material",_material->getMaterialMatrix());
    s->Uniform("opacity", _material->getOpacityValue());
    s->Uniform("textureCount", _material->getTextureCount());
    s->Uniform("isSelected", sgn->isSelected() ? 1 : 0);

    if(lightMgr.shadowMappingEnabled()){
        s->Uniform("worldHalfExtent", lightMgr.getLigthOrthoHalfExtent());
        s->Uniform("dvd_lightProjectionMatrices",lightMgr.getLightProjectionMatricesCache());
        s->Uniform("dvd_enableShadowMapping",_material->getReceivesShadows());
    }else{
        s->Uniform("dvd_enableShadowMapping",false);
    }

    s->Uniform("dvd_lightType",        lightMgr.getLightTypesForCurrentNode());
    s->Uniform("dvd_lightCount",       lightMgr.getLightCountForCurrentNode());
    s->Uniform("dvd_lightCastsShadows",lightMgr.getShadowCastingLightsForCurrentNode());

    s->Uniform("windDirection",vec2<F32>(activeScene->state().getWindDirX(),activeScene->state().getWindDirZ()));
    s->Uniform("windSpeed", activeScene->state().getWindSpeed());

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

    Texture2D* texture = NULL;
    for(U16 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        if((texture = _material->getTexture(i)) != NULL)
            texture->Unbind(i);
}

void SceneNode::prepareDepthMaterial(SceneGraphNode* const sgn){
    if(getType() != TYPE_OBJECT3D && getType() != TYPE_TERRAIN)
        return;

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

    if(_material->isTranslucent()){
    
        Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
        if(opacityMap)
            opacityMap->Bind(Material::TEXTURE_OPACITY);
        else{
            // maybe the diffuse texture has an alpha channel so use it as an opacity map
            Texture2D* diffuse = _material->getTexture(Material::TEXTURE_UNIT0);
            diffuse->Bind(Material::TEXTURE_OPACITY);
        }

        s->Uniform("opacity", _material->getOpacityValue());
    }

    if(!sgn->animationTransforms().empty()){
        s->Uniform("hasAnimations", true);
        s->Uniform("boneTransforms", sgn->animationTransforms());
    }else{
        s->Uniform("hasAnimations",false);
    }
}

void SceneNode::releaseDepthMaterial(){
    //UpgradableReadLock ur_lock(_materialLock);
    if(!_material)
        return;

    if(_material->isTranslucent()){
    
        Texture2D* opacityMap = _material->getTexture(Material::TEXTURE_OPACITY);
        if(opacityMap)
            opacityMap->Unbind(Material::TEXTURE_OPACITY);
        else{
            // maybe the diffuse texture has an alpha channel so use it as an opacity map
            Texture2D* diffuse = _material->getTexture(Material::TEXTURE_UNIT0);
            diffuse->Unbind(Material::TEXTURE_OPACITY);
        }

    }
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
    const BoundingBox& bb = sgn->getBoundingBox();
    GFX_DEVICE.drawBox3D(bb.getMin(),bb.getMax(),mat4<F32>());
}