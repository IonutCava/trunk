#include "stdafx.h"

#include "Headers/SceneNode.h"

#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"

#include "ECS/Components/Headers/BoundsComponent.h"

namespace Divide {

namespace {
    bool isRigidBody(SceneNode& node) {
        STUBBED("Due to the way that physics libs work in world transforms only, a classical scene graph type system does not "
            "fit the physics model well. Thus, for now, only first-tier nodes are physically simulated!");
        // If nodes need more complex physical relationships, they can be added as first tier nodes and linked together
        // with a joint type system and the "parent" node's 'relative' mass set to infinite so the child node couldn't move it
        if (node.type() == SceneNodeType::TYPE_OBJECT3D) {
            if (static_cast<Object3D&>(node).getObjectType()._value != ObjectType::SUBMESH) {
                return true;
            }
        }
        return false;
    }
};

SceneNode::SceneNode(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const SceneNodeType& type)
    : SceneNode(parentCache, descriptorHash, name, name, "", type)
{
}

SceneNode::SceneNode(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, const SceneNodeType& type)
    : CachedResource(ResourceType::DEFAULT, descriptorHash, name, resourceName, resourceLocation),
     _parentCache(parentCache),
     _materialTemplate(nullptr),
     _type(type),
     _LODcount(1),
     _editorComponent("")
{
    getEditorComponent().name(getTypeName());
    getEditorComponent().onChangedCbk([this](EditorComponentField& field) {
        editorFieldChanged(field);
    });

}

SceneNode::~SceneNode()
{
}

const char* SceneNode::getTypeName() const {
    switch (_type) {
        case SceneNodeType::TYPE_ROOT: return "ROOT";
        case SceneNodeType::TYPE_OBJECT3D: return "OBJECT3D";
        case SceneNodeType::TYPE_TRANSFORM: return "TRANSFORM";
        case SceneNodeType::TYPE_WATER: return "WATER";
        case SceneNodeType::TYPE_TRIGGER: return "TRIGGER";
        case SceneNodeType::TYPE_PARTICLE_EMITTER: return "PARTICLE_EMITTER";
        case SceneNodeType::TYPE_SKY: return "SKY";
        case SceneNodeType::TYPE_VEGETATION_GRASS: return "VEGETATION_GRASS";
        case SceneNodeType::TYPE_VEGETATION_TREES: return "VEGETATION_TREES";
    }

    return "";
}

void SceneNode::frameStarted(SceneGraphNode& sgn) {
    ACKNOWLEDGE_UNUSED(sgn);
}

void SceneNode::frameEnded(SceneGraphNode& sgn) {
    ACKNOWLEDGE_UNUSED(sgn);
}

void SceneNode::sceneUpdate(const U64 deltaTimeUS,
                            SceneGraphNode& sgn,
                            SceneState& sceneState) {
    ACKNOWLEDGE_UNUSED(deltaTimeUS);
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(sceneState);
}

bool SceneNode::onRender(SceneGraphNode& sgn,
                         const SceneRenderState& sceneRenderState,
                         RenderStagePass renderStagePass) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(sceneRenderState);
    ACKNOWLEDGE_UNUSED(renderStagePass);

    if (getState() == ResourceState::RES_LOADED) {
        return true;
    }
    return false;
}

void SceneNode::updateBoundsInternal() {
}

void SceneNode::postLoad(SceneGraphNode& sgn) {
    BoundsComponent* bComp = sgn.get<BoundsComponent>();
    if (bComp) {
        bComp->boundsChanged();
    }
    sgn.postLoad();
}

void SceneNode::setBoundsChanged() {
    for (SceneGraphNode* parent : _sgnParents) {
        assert(parent != nullptr);
        
        BoundsComponent* bComp = parent->get<BoundsComponent>();
        if (bComp) {
            bComp->boundsChanged();
        }
    }
}

bool SceneNode::getDrawState(RenderStagePass currentStagePass) {
    return _renderState.getDrawState(currentStagePass);
}

const Material_ptr& SceneNode::getMaterialTpl() const {
    // UpgradableReadLock ur_lock(_materialLock);
    return _materialTemplate;
}

void SceneNode::setMaterialTpl(const Material_ptr& material) {
    if (material) {  // If we need to update the material
        // UpgradableReadLock ur_lock(_materialLock);

        // If we had an old material
        if (_materialTemplate) {  
            // if the old material isn't the same as the new one
            if (_materialTemplate->getGUID() != material->getGUID()) {
                Console::printfn(Locale::get(_ID("REPLACE_MATERIAL")),
                                 _materialTemplate->name().c_str(),
                                 material->name().c_str());
                _materialTemplate = material;  // set the new material
            }
        } else {
            _materialTemplate = material;  // set the new material
        }
    } else {  // if we receive a null material, the we need to remove this node's material
        _materialTemplate.reset();
    }
}

bool SceneNode::unload() {
    setMaterialTpl(nullptr);
    return true;
}

void SceneNode::editorFieldChanged(EditorComponentField& field) {
    ACKNOWLEDGE_UNUSED(field);
}

void SceneNode::buildDrawCommands(SceneGraphNode& sgn,
                                       RenderStagePass renderStagePass,
                                       RenderPackage& pkgInOut) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(renderStagePass);
    ACKNOWLEDGE_UNUSED(pkgInOut);
}

void SceneNode::onCameraUpdate(SceneGraphNode& sgn,
                               I64 cameraGUID,
                               const vec3<F32>& posOffset,
                               const mat4<F32>& rotationOffset) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(cameraGUID);
    ACKNOWLEDGE_UNUSED(posOffset);
    ACKNOWLEDGE_UNUSED(rotationOffset);
}

void SceneNode::onNetworkSend(SceneGraphNode& sgn, WorldPacket& dataOut) const {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(dataOut);
}

void SceneNode::onNetworkReceive(SceneGraphNode& sgn, WorldPacket& dataIn) const {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(dataIn);
}

void SceneNode::saveCache(ByteBuffer& outputBuffer) const {
    if (type() == SceneNodeType::TYPE_OBJECT3D) {
        if (static_cast<const Object3D*>(this)->isPrimitive()) {
            outputBuffer << static_cast<const Object3D*>(this)->getObjectType()._to_string();
        } else {
            outputBuffer << name();
        }
    }
}

void SceneNode::loadCache(ByteBuffer& inputBuffer) {
    ACKNOWLEDGE_UNUSED(inputBuffer);
}

void SceneNode::saveToXML(boost::property_tree::ptree& pt) const {
    ACKNOWLEDGE_UNUSED(pt);
}

void SceneNode::loadFromXML(const boost::property_tree::ptree& pt) {
    ACKNOWLEDGE_UNUSED(pt);
}

void Attorney::SceneNodeSceneGraph::registerSGNParent(SceneNode& node, SceneGraphNode* sgn) {
    // prevent double add
    I64 targetGUID = sgn ? -1 : sgn->getGUID();
    vector<SceneGraphNode*>::const_iterator it;
    it = std::find_if(std::cbegin(node._sgnParents), std::cend(node._sgnParents), [targetGUID](SceneGraphNode* sgn) {
        return sgn && sgn->getGUID() == targetGUID;
    });
    assert(it == std::cend(node._sgnParents));

    node._sgnParents.push_back(sgn);
}

void Attorney::SceneNodeSceneGraph::unregisterSGNParent(SceneNode& node, SceneGraphNode* sgn) {
    // prevent double remove
    I64 targetGUID = sgn ? sgn->getGUID() : -1;
    vector<SceneGraphNode*>::const_iterator it;
    it = std::find_if(std::cbegin(node._sgnParents), std::cend(node._sgnParents), [targetGUID](SceneGraphNode* sgn) {
        return sgn && sgn->getGUID() == targetGUID;
    });
    assert(it != std::cend(node._sgnParents));

    node._sgnParents.erase(it);
}

};