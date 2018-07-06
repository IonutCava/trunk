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

    // update transform
    if (getTransform())
        _transform->setParentTransform(_parent ? _parent->getTransform() : nullptr);
        
    // update all of the internal components (animation, physics, etc)
    FOR_EACH(NodeComponents::value_type& it, _components)
        if (it.second) it.second->update(deltaTime);

    assert(_node->getState() == RES_LOADED);
    //Update order is very important! e.g. Mesh BB is composed of SubMesh BB's.

    //Compute the BoundingBox if it isn't already
    if (!_boundingBox.isComputed()) {
        _node->computeBoundingBox(this);
        assert(_boundingBox.isComputed());
        _boundingBoxDirty = true;
    }

    if (_boundingBoxDirty){
        if (_transform) updateBoundingBoxTransform(getWorldMatrix());
        if (_parent)    _parent->getBoundingBox().setComputed(false);

        _boundingBoxDirty = false;
    }

    _node->sceneUpdate(deltaTime, this, sceneState);

    Material* mat = _node->getMaterial();
    if (mat)
        mat->update(deltaTime);
    

    if(_shouldDelete) GET_ACTIVE_SCENEGRAPH()->addToDeletionQueue(this);
}

void SceneGraphNode::render(const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    //Call any pre-draw operations on the SceneGraphNode (e.g. tick animations)
    //Check if we should draw the node. (only after onDraw as it may contain exclusion mask changes before draw)
    if(!onDraw(currentRenderStage))
        return; //< If the SGN isn't ready for rendering, skip it this frame
    
    _node->bindTextures();
    _node->render(this, sceneRenderState, currentRenderStage);

    postDraw(currentRenderStage);
}

bool SceneGraphNode::onDraw(RenderStage renderStage){
    if (_drawReset[renderStage]){
        _drawReset[renderStage] = false;
        if (getParent() && !bitCompare(DEPTH_STAGE, renderStage)){
            FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, getParent()->getChildren())
                if (it.second->getComponent<AnimationComponent>()) 
                    it.second->getComponent<AnimationComponent>()->reset();
        }
            
    }

    FOR_EACH(NodeComponents::value_type& it, _components)
        if (it.second) it.second->onDraw(renderStage);
    
    //Call any pre-draw operations on the SceneNode (refresh VB, update materials, etc)
    Material* mat = _node->getMaterial();
    if (mat){
        if (mat->computeShader(renderStage)){
            scheduleDrawReset(renderStage); //reset animation on next draw call
            return false;
        }
    }

    if(_node->onDraw(this, renderStage))
        return _node->getDrawState(renderStage);

    return false;
}

void SceneGraphNode::postDraw(RenderStage renderStage){
    // Perform any post draw operations regardless of the draw state
    _node->postDraw(this, renderStage);
}

void SceneGraphNode::isInViewCallback(){
    if(!_inView)
        return;

    _materialColorMatrix.zero();
    _materialPropertyMatrix.zero();

    _materialPropertyMatrix.setCol(0, vec4<F32>(isSelected() ? 1.0f : 0.0f, getReceivesShadows() ? 1.0f : 0.0f, 0.0f, 0.0f));

    Material* mat = _node->getMaterial();
    
    if (mat){
        mat->getMaterialMatrix(_materialColorMatrix);
        _materialPropertyMatrix.setCol(1, vec4<F32>((mat->isTranslucent() ? (mat->useAlphaTest() || GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE)) : false) ? 1.0f : 0.0f,
                                                    (F32)mat->getTextureOperation(), (F32)mat->getTextureCount(), 0.0f));
    }

}
