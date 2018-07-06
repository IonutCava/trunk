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
                                                             _drawShader(nullptr),
                                                             _refreshMaterialData(true),
                                                             _nodeReady(false),
                                                             _type(type),
                                                             _lodLevel(0),
                                                             _drawStateHash(0),
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

    return mat->getShaderInfo(currentStage).getProgram()->isHWInitComplete();
}

bool SceneNode::isInView(const SceneRenderState& sceneRenderState, const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck){
    assert(_nodeReady);
    
    const Camera& cam = sceneRenderState.getCameraConst();
    const vec3<F32>& eye = cam.getEye();
    const vec3<F32>& center  = sphere.getCenter();
    F32  cameraDistance = center.distance(eye);
    F32 visibilityDistance = GET_ACTIVE_SCENE()->state().getGeneralVisibility() + sphere.getRadius();
    if(distanceCheck && cameraDistance > visibilityDistance){
        if (boundingBox.nearestDistanceFromPointSquared(eye) > std::min(visibilityDistance, sceneRenderState.getCameraConst().getZPlanes().y))
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

    Material::ShaderInfo& shaderInfo = _material->getShaderInfo();
    _drawShader = shaderInfo.getProgram();

    LightManager& lightMgr = LightManager::getInstance();

    if (!_drawShader->bind())
        return false;

    _drawShader->ApplyMaterial(_material);
    _drawShader->SetLOD(getCurrentLOD());

    bool temp = sgn->isSelected();
    if(shaderInfo.getTrackedBool(0) != temp){
        shaderInfo.setTrackedBool(0, temp);
        _drawShader->Uniform("dvd_isSelected", temp);
    }
    
    temp = lightMgr.shadowMappingEnabled() && sgn->getReceivesShadows();
    if(shaderInfo.getTrackedBool(1) != temp){
        shaderInfo.setTrackedBool(1, temp);
        _drawShader->Uniform("dvd_enableShadowMapping", temp);
    }        

    AnimationComponent* animComponent = sgn->getComponent<AnimationComponent>();
    temp = animComponent != nullptr && !animComponent->animationTransforms().empty();
    if(temp){
        _drawShader->Uniform("dvd_boneOffset", (I32)(sgn->getInstanceID() * animComponent->animationTransforms().size()));
    }
    if(shaderInfo.getTrackedBool(2) != temp){
        shaderInfo.setTrackedBool(2, temp);
        _drawShader->Uniform("dvd_hasAnimations", temp);
    }
        

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

    Material::ShaderInfo& shaderInfo = _material->getShaderInfo(shadowStage ? SHADOW_STAGE : Z_PRE_PASS_STAGE);
    _drawShader = shaderInfo.getProgram();

    assert(_drawShader != nullptr);

    if (!_drawShader->bind())
        return false;

    _drawShader->ApplyMaterial(_material);
    _drawShader->SetLOD(getCurrentLOD());
    if (_material->isTranslucent()){
        if (_material->getTexture(Material::TEXTURE_OPACITY)) 
            _material->getTexture(Material::TEXTURE_OPACITY)->Bind(Material::TEXTURE_OPACITY);
        if (_material->getTexture(Material::TEXTURE_UNIT0))
            _material->getTexture(Material::TEXTURE_UNIT0)->Bind(Material::TEXTURE_UNIT0);
    }

    AnimationComponent* animComponent = sgn->getComponent<AnimationComponent>();
    bool temp = animComponent != nullptr && !animComponent->animationTransforms().empty();
    if(temp){
        _drawShader->Uniform("dvd_boneOffset", (I32)(sgn->getInstanceID() * animComponent->animationTransforms().size()));
    }
    if(shaderInfo.getTrackedBool(2) != temp){
        shaderInfo.setTrackedBool(2, temp);
        _drawShader->Uniform("dvd_hasAnimations", temp);
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