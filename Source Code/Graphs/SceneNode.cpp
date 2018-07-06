#include "Headers/SceneNode.h"

#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"

SceneNode::SceneNode(const SceneNodeType& type) : SceneNode("default", type)
{
}

SceneNode::SceneNode(const std::string& name, const SceneNodeType& type) : Resource(name),
                                                             _material(nullptr),
                                                             _customShader(nullptr),
                                                             _refreshMaterialData(true),
                                                             _nodeReady(false),
                                                             _type(type),
                                                             _lodLevel(0),
                                                             _LODcount(1), ///<Defaults to 1 LOD level
                                                             _sgnReferenceCount(0),
                                                             _physicsAsset(nullptr)
{
}

SceneNode::~SceneNode() {
    SAFE_DELETE(_physicsAsset);
}

void SceneNode::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    assert(_nodeReady);

    if(!_material) return;

    _refreshMaterialData = _material->isDirty();
    _material->clean();
}

void SceneNode::preFrameDrawEnd(SceneGraphNode* const sgn) {
    assert(_nodeReady);
    if (!sgn->inView()) return;

    //draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifacts
    //Draw the bounding box if it's always on or if the scene demands it
    if (sgn->getBoundingBoxConst().getVisibility() || GET_ACTIVE_SCENE()->renderState().drawBBox()){
        drawBoundingBox(sgn);
    }
    if (sgn->getComponent<AnimationComponent>()){
        sgn->getComponent<AnimationComponent>()->renderSkeleton();
    }
}
bool SceneNode::isReadyForDraw(const RenderStage& currentStage){
    Material* mat = getMaterial();
    if (!mat)
        return true;

    return mat->getShaderProgram(currentStage)->isHWInitComplete();
}

bool SceneNode::isInView(const SceneRenderState& sceneRenderState, const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck){
    assert(_nodeReady);
    
    const Camera& cam = sceneRenderState.getCameraConst();
    const vec3<F32>& eye = cam.getEye();
    const vec3<F32>& center  = sphere.getCenter();
    F32  cameraDistance = center.distance(eye);
    F32 visibilityDistance = GET_ACTIVE_SCENE()->state().getGeneralVisibility() + sphere.getRadius();
    if(distanceCheck && cameraDistance > visibilityDistance){
        if(boundingBox.nearestDistanceFromPointSquared(eye) > visibilityDistance)
            return false;
    }

    if(!boundingBox.ContainsPoint(eye)){
        switch (cam.getFrustumConst().ContainsSphere(center, sphere.getRadius())) {
            case Frustum::FRUSTUM_OUT: return false;
            case Frustum::FRUSTUM_INTERSECT:	{
                if (!cam.getFrustumConst().ContainsBoundingBox(boundingBox)) return false;
            }
        }
    }

    _lodLevel = (cameraDistance > Config::SCENE_NODE_LOD0) ? ((cameraDistance > Config::SCENE_NODE_LOD1) ? 2 : 1) : 0;
    
    return true;
}

Material* SceneNode::getMaterial(){
    //UpgradableReadLock ur_lock(_materialLock);
    if(_material == nullptr){
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
            if(_material->getGUID() != m->getGUID()){ //if the old material isn't the same as the new one
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

bool SceneNode::prepareMaterial(SceneGraphNode* const sgn){
    assert(_nodeReady);

    //UpgradableReadLock ur_lock(_materialLock);
    if(!_material)
        return true;

    if(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE))
        SET_STATE_BLOCK(_material->getRenderStateBlock(REFLECTION_STAGE));
    else
        SET_STATE_BLOCK(_material->getRenderStateBlock(FINAL_STAGE));

    ShaderProgram* s = _material->getShaderProgram();
    Scene* activeScene = GET_ACTIVE_SCENE();
    LightManager& lightMgr = LightManager::getInstance();

    Texture* texture = nullptr;
    for (U16 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i){
        if ((texture = _material->getTexture(i)) != nullptr){
            texture->Bind(i);
        }
    }

    if(!s->bind())
        return false;

    s->ApplyMaterial(_material);
    s->SetLOD(getCurrentLOD());
    s->Uniform("isSelected", sgn->isSelected() ? 1 : 0);
    s->Uniform("dvd_enableShadowMapping", lightMgr.shadowMappingEnabled() && sgn->getReceivesShadows());
    s->Uniform("dvd_lightIndex",          lightMgr.getLightIndicesForCurrentNode());
    s->Uniform("dvd_lightType",           lightMgr.getLightTypesForCurrentNode());
    s->Uniform("dvd_lightCount",          lightMgr.getLightCountForCurrentNode());
    s->Uniform("dvd_lightCastsShadows",   lightMgr.getShadowCastingLightsForCurrentNode());

    s->Uniform("windDirection",vec2<F32>(activeScene->state().getWindDirX(),activeScene->state().getWindDirZ()));
    s->Uniform("windSpeed", activeScene->state().getWindSpeed());

    AnimationComponent* animComponent = sgn->getComponent<AnimationComponent>();
    if (animComponent){
        const vectorImpl<mat4<F32> >& boneTransforms = animComponent->animationTransforms();
        s->Uniform("dvd_hasAnimations", !boneTransforms.empty());
        s->Uniform("boneTransforms", boneTransforms);
    }else{
        s->Uniform("dvd_hasAnimations", false);
    }

    return true;
}

bool SceneNode::releaseMaterial(){
    assert(_nodeReady);

    //UpgradableReadLock ur_lock(_materialLock);
    if(!_material) return true;

    Texture* texture = nullptr;
    for(U16 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        if((texture = _material->getTexture(i)) != nullptr)
            texture->Unbind(i);

    return true;
}

bool SceneNode::prepareDepthMaterial(SceneGraphNode* const sgn){
    assert(_nodeReady);

    if(getType() != TYPE_OBJECT3D && getType() != TYPE_TERRAIN)
        return true;

    bool shadowStage = GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE);
    
    //UpgradableReadLock ur_lock(_materialLock);
    if(!_material) {
        SET_STATE_BLOCK(shadowStage ? _renderState.getShadowStateBlock() : _renderState.getDepthStateBlock());
        return true;
    }
    SET_STATE_BLOCK(_material->getRenderStateBlock(shadowStage ? SHADOW_STAGE : Z_PRE_PASS_STAGE));

    ShaderProgram* s = _material->getShaderProgram(shadowStage ? SHADOW_STAGE : Z_PRE_PASS_STAGE);
    assert(s != nullptr);

    if (!s->bind())
        return false;

    s->ApplyMaterial(_material);
    s->SetLOD(getCurrentLOD());
    if (_material->isTranslucent()){
        switch (_material->getTranslucencySource()){
        case Material::TRANSLUCENT_OPACITY_MAP:
            _material->getTexture(Material::TEXTURE_OPACITY)->Bind(Material::TEXTURE_OPACITY);
            break;
        case Material::TRANSLUCENT_DIFFUSE_MAP:
            _material->getTexture(Material::TEXTURE_UNIT0)->Bind(Material::TEXTURE_UNIT0);
            break;
        };
    }

    AnimationComponent* animComponent = sgn->getComponent<AnimationComponent>();
    if (animComponent){
        const vectorImpl<mat4<F32> >& boneTransforms = animComponent->animationTransforms();
        s->Uniform("dvd_hasAnimations", !boneTransforms.empty());
        s->Uniform("boneTransforms", boneTransforms);
    }
    else{
        s->Uniform("dvd_hasAnimations", false);
    }

    return true;
}

bool SceneNode::releaseDepthMaterial(){
    assert(_nodeReady);

    //UpgradableReadLock ur_lock(_materialLock);
    if(!_material || !_material->isTranslucent())
        return true;

    if (_material->isTranslucent()){
        switch (_material->getTranslucencySource()){
        case Material::TRANSLUCENT_OPACITY_MAP:
            _material->getTexture(Material::TEXTURE_OPACITY)->Unbind(Material::TEXTURE_OPACITY);
            break;
        case Material::TRANSLUCENT_DIFFUSE_MAP:
            _material->getTexture(Material::TEXTURE_UNIT0)->Unbind(Material::TEXTURE_UNIT0);
            break;
        };
    }

    return true;
}

bool SceneNode::computeBoundingBox(SceneGraphNode* const sgn) {
    sgn->setInitialBoundingBox(sgn->getBoundingBoxConst());
    sgn->getBoundingBox().setComputed(true);
    return true;
}

bool SceneNode::unload(){
    setMaterial(nullptr);
    return true;
}

void SceneNode::drawBoundingBox(SceneGraphNode* const sgn) const {
    const BoundingBox& bb = sgn->getBoundingBoxConst();
    GFX_DEVICE.drawBox3D(bb.getMin(),bb.getMax(),mat4<F32>());
}