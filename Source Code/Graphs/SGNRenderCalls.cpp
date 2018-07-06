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

void SceneGraphNode::setSelected(bool state) {
    _selected = state;
     FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->setSelected(_selected);
    }
}

bool SceneGraphNode::getCastsShadows() const {
    return _castsShadows && LightManager::getInstance().shadowMappingEnabled();
}

bool SceneGraphNode::getReceivesShadows() const {
    return _receiveShadows && LightManager::getInstance().shadowMappingEnabled();
}

void SceneGraphNode::updateBoundingBoxTransform(const mat4<F32>& transform){
    if (_boundingBox.Transform(_initialBoundingBox, transform, !_initialBoundingBox.Compare(_initialBoundingBoxCache)))
        _initialBoundingBoxCache = _initialBoundingBox;
        _boundingSphere.fromBoundingBox(_boundingBox);
}

void SceneGraphNode::setInitialBoundingBox(const BoundingBox& initialBoundingBox){
    if (!initialBoundingBox.Compare(getInitialBoundingBox())){
        _initialBoundingBox = initialBoundingBox;
        _initialBoundingBox.setComputed(true);
        _boundingBoxDirty = true;
    }
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

    // update transform
    if (getTransform()) {
        _transform->setParentTransform(_parent ? _parent->getTransform() : nullptr);
        _transform->update(deltaTime);
    }
    
    // update all of the internal components (animation, physics, etc)
    FOR_EACH(NodeComponents::value_type& it, _components)
        if (it.second) it.second->update(deltaTime);

    if (_node) {
        assert(_node->getState() == RES_LOADED);
        //Update order is very important! e.g. Mesh BB is composed of SubMesh BB's.

        //Compute the BoundingBox if it isn't already
        if (!_boundingBox.isComputed()) {
            _node->computeBoundingBox(this);
            assert(_boundingBox.isComputed());
            _boundingBoxDirty = true;
        }

        if (_boundingBoxDirty){
            if (_transform) updateBoundingBoxTransform(_transform->getGlobalMatrix());
            if (_parent)    _parent->getBoundingBox().setComputed(false);

            _boundingBoxDirty = false;
        }

        _node->sceneUpdate(deltaTime, this, sceneState);

        Material* mat = _node->getMaterial();
        if (mat) mat->update(deltaTime);
    }

    if(_shouldDelete) GET_ACTIVE_SCENEGRAPH()->addToDeletionQueue(this);
}

bool SceneGraphNode::onDraw(RenderStage renderStage){
    if (_drawReset[renderStage]){
        _drawReset[renderStage] = false;
        if (getParent() && !bitCompare(DEPTH_STAGE, renderStage)){
            FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, getParent()->getChildren())
                if (it.second->getComponent<AnimationComponent>()) it.second->getComponent<AnimationComponent>()->reset();
        }
            
    }

    FOR_EACH(NodeComponents::value_type& it, _components)
        if (it.second) it.second->onDraw(renderStage);
    
    //Call any pre-draw operations on the SceneNode (refresh VB, update materials, etc)
    if (_node){
        Material* mat = _node->getMaterial();
        if (mat){
            if (mat->computeShader(false, renderStage)){
                scheduleDrawReset(renderStage); //reset animation on next draw call
                return false;
            }
        }
        return _node->onDraw(this, renderStage);
    }

    return true;
}

void SceneGraphNode::postDraw(RenderStage renderStage){
    // Perform any post draw operations regardless of the draw state
    if (_node) _node->postDraw(this, renderStage);
}

void SceneGraphNode::updateShaderData(const mat4<F32>& viewMatrix, const D32 interpolationFactor){
    Transform* transform = getTransform();
    if (transform) {
        _nodeShaderData._worldMatrix.set(transform->interpolate(getPrevTransform(), interpolationFactor));
        _nodeShaderData._normalMatrix.set(_nodeShaderData._worldMatrix * viewMatrix);

        if(!transform->isUniformScaled())
            _nodeShaderData._normalMatrix.inverseTranspose();
    }else{
        _nodeShaderData._worldMatrix.identity();
        _nodeShaderData._normalMatrix.identity();
    }

    _nodeShaderData._integerValues.x = isSelected() ? 1 : 0;
    _nodeShaderData._integerValues.y = getReceivesShadows() ? 1 : 0;

    AnimationComponent* animComponent = getComponent<AnimationComponent>();
    if(animComponent && !animComponent->animationTransforms().empty()){
        _nodeShaderData._integerValues.z = (U32)(getInstanceID() * animComponent->animationTransforms().size());
    }

    _nodeShaderData._lightInfo.set(LightManager::getInstance().findLightsForSceneNode(this), 0, 0, 0);
}