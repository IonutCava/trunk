#include "Headers/SceneGraphNode.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

bool SceneRoot::computeBoundingBox(SceneGraphNode* const sgn) {
    BoundingBox& bb = sgn->getBoundingBox();
    bb.reset();
    FOR_EACH(SceneGraphNode::NodeChildren::value_type& s, sgn->getChildren()){
        sgn->addBoundingBox(s.second->getBoundingBoxConst(), s.second->getNode()->getType());
    }
    bb.setComputed(true);
    return SceneNode::computeBoundingBox(sgn);
}

void SceneGraphNode::setSelected(const bool state) {
    _selected = state;
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->setSelected(_selected);
    }
}

void SceneGraphNode::renderSkeleton(const bool state) {
    _renderSkeleton = state;
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->renderSkeleton(_renderSkeleton);
    }
}

void SceneGraphNode::renderWireframe(const bool state) {
    _renderWireframe = state;
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->renderWireframe(_renderWireframe);
    }
}

void SceneGraphNode::renderBoundingBox(const bool state) {
    _renderBoundingBox = state; 
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->renderBoundingBox(_renderBoundingBox);
    }
}

void SceneGraphNode::castsShadows(const bool state)    {
    _castsShadows = state;
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->castsShadows(_castsShadows);
    }
}

void SceneGraphNode::receivesShadows(const bool state) {
    _receiveShadows = state;
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->receivesShadows(_receiveShadows);
    }
}

bool SceneGraphNode::castsShadows() const {
    return _castsShadows && LightManager::getInstance().shadowMappingEnabled();
}

bool SceneGraphNode::receivesShadows() const {
    return _receiveShadows && LightManager::getInstance().shadowMappingEnabled();
}

bool SceneGraphNode::updateBoundingBoxTransform(const mat4<F32>& transform){
    if (_boundingBox.Transform(_initialBoundingBox, transform, !_initialBoundingBox.Compare(_initialBoundingBoxCache))){
        _initialBoundingBoxCache = _initialBoundingBox;
        _boundingSphere.fromBoundingBox(_boundingBox);
        return true;
    }
    return false;
}

void SceneGraphNode::setInitialBoundingBox(const BoundingBox& initialBoundingBox){
    if (!initialBoundingBox.Compare(getInitialBoundingBox())){
        _initialBoundingBox = initialBoundingBox;
        _initialBoundingBox.setComputed(true);
        _boundingBoxDirty = true;
    }
}

void SceneGraphNode::onCameraChange(){
    FOR_EACH(NodeChildren::value_type& it, _children)
        it.second->onCameraChange();
    
    _node->onCameraChange(this);
}

///Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    //Compute from leaf to root to ensure proper calculations
    FOR_EACH(NodeChildren::value_type& it, _children){
        assert(it.second);
        it.second->sceneUpdate(deltaTime, sceneState);
    }
    // update local time
    _elapsedTime += deltaTime;
    Transform* transform = getComponent<PhysicsComponent>()->getTransform();
    // update transform
    if (transform) {
        transform->setParentTransform(_parent ? _parent->getComponent<PhysicsComponent>()->getTransform() : nullptr);
    }
    // update all of the internal components (animation, physics, etc)
    FOR_EACH(NodeComponents::value_type& it, _components) {
        if (it.second) {
            it.second->update(deltaTime);
        }
    }
    if (getComponent<PhysicsComponent>()->transformUpdated()){
        _boundingBoxDirty = true;
        FOR_EACH(NodeChildren::value_type& it, _children){
            it.second->getComponent<PhysicsComponent>()->transformUpdated(true);
        }
    }
    assert(_node->getState() == RES_LOADED);
    //Update order is very important! e.g. Mesh BB is composed of SubMesh BB's.

    //Compute the BoundingBox if it isn't already
    if (!_boundingBox.isComputed()) {
        _node->computeBoundingBox(this);
        assert(_boundingBox.isComputed());
        _boundingBoxDirty = true;
    }

    if (_boundingBoxDirty) {
        if (updateBoundingBoxTransform(getWorldMatrix())) {
            if (_parent) {
                _parent->getBoundingBox().setComputed(false);
            }
        }
        _boundingBoxDirty = false;
    }

    getComponent<PhysicsComponent>()->transformUpdated(false);

    _node->sceneUpdate(deltaTime, this, sceneState);

    Material* mat = _node->getMaterial();
    if (mat) {
        mat->update(deltaTime);
    }

    if (_shouldDelete) {
        GET_ACTIVE_SCENEGRAPH()->addToDeletionQueue(this);
    }
}

void SceneGraphNode::render(const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    // Call any pre-draw operations on the SceneGraphNode (e.g. tick animations)
    // Check if we should draw the node. (only after onDraw as it may contain exclusion mask changes before draw)
    if (!onDraw(sceneRenderState, currentRenderStage)) {
        // If the SGN isn't ready for rendering, skip it this frame
        return; 
    }
    _node->bindTextures();
    _node->render(this, sceneRenderState, currentRenderStage);

    postDraw(sceneRenderState, currentRenderStage);
}

bool SceneGraphNode::onDraw(const SceneRenderState& sceneRenderState, RenderStage renderStage){
    if (_drawReset[renderStage]) {
        _drawReset[renderStage] = false;
        if (getParent() && !GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE)) {
            FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, getParent()->getChildren()) {
                if (it.second->getComponent<AnimationComponent>()) {
                    it.second->getComponent<AnimationComponent>()->reset();
                }
            }
        }
            
    }

    FOR_EACH(NodeComponents::value_type& it, _components) {
        if (it.second) {
            it.second->onDraw(renderStage);
        }
    }
    //Call any pre-draw operations on the SceneNode (refresh VB, update materials, etc)
    Material* mat = _node->getMaterial();
    if (mat){
        if (mat->computeShader(renderStage)){
            scheduleDrawReset(renderStage); //reset animation on next draw call
            return false;
        }
    }

    if (_node->onDraw(this, renderStage)) {
        return _node->getDrawState(renderStage);
    }
    return false;
}

void SceneGraphNode::postDraw(const SceneRenderState& sceneRenderState, RenderStage renderStage){
    // Perform any post draw operations regardless of the draw state
    _node->postDraw(this, renderStage);

#ifdef _DEBUG
    if (sceneRenderState.gizmoState() == SceneRenderState::ALL_GIZMO) {
        if (_node->getType() == SceneNodeType::TYPE_OBJECT3D) {
            if (dynamic_cast<Object3D*>(_node)->getObjectType() == Object3D::MESH) {
                drawDebugAxis();
            }
        }
    } else {
        if (!isSelected()) {
            _axisGizmo->paused(true);   
        }
    }

    // Draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifacts
    if (renderBoundingBox() || bitCompare(sceneRenderState.objectState(), SceneRenderState::DRAW_BOUNDING_BOX)) {
        _node->drawBoundingBox(this);
    }
#endif

    if (getComponent<AnimationComponent>()) {
        getComponent<AnimationComponent>()->renderSkeleton();
    }
}

void SceneGraphNode::isInViewCallback(){
    if (!_inView) {
        return;
    }

    _materialColorMatrix.zero();
    _materialPropertyMatrix.zero();

    _materialPropertyMatrix.setCol(0, vec4<F32>(isSelected() ? 1.0f : 0.0f, receivesShadows() ? 1.0f : 0.0f, 0.0f, 0.0f));

    Material* mat = _node->getMaterial();
    
    if (mat){
        mat->getMaterialMatrix(_materialColorMatrix);
        _materialPropertyMatrix.setCol(1, vec4<F32>((mat->isTranslucent() ? (mat->useAlphaTest() || GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE)) : false) ? 1.0f : 0.0f,
                                                    (F32)mat->getTextureOperation(), (F32)mat->getTextureCount(), 0.0f));
    }

}

#ifdef _DEBUG
/// Draw the axis arrow gizmo
void SceneGraphNode::drawDebugAxis() {
    const Transform* transform = getComponent<PhysicsComponent>()->getConstTransform();
    if (transform) {
        mat4<F32> tempOffset(::getMatrix(transform->getOrientation()));
        tempOffset.setTranslation(transform->getPosition());
        _axisGizmo->worldMatrix(tempOffset);
    } else {
        _axisGizmo->resetWorldMatrix();
    }
    _axisGizmo->paused(false);
}
#endif