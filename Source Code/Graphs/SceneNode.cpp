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

SceneNode::SceneNode(ResourceCache* parentCache, size_t descriptorHash, const Str128& name, const Str128& resourceName, const stringImpl& resourceLocation, SceneNodeType type, U32 requiredComponentMask)
    : CachedResource(ResourceType::DEFAULT, descriptorHash, name, resourceName, resourceLocation),
     _parentCache(parentCache),
     _type(type),
     _editorComponent("")
{
    std::atomic_init(&_sgnParentCount, 0);
    _requiredComponentMask |= requiredComponentMask;

    _boundingBox.setMin(-1.0f);
    _boundingBox.setMax(1.0f);

    getEditorComponent().name(getTypeName());
    getEditorComponent().onChangedCbk([this](const char* field) {
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
        case SceneNodeType::TYPE_INFINITEPLANE: return "INFINITE_PLANE";
        case SceneNodeType::TYPE_VEGETATION: return "VEGETATION_GRASS";
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

bool SceneNode::prepareRender(SceneGraphNode& sgn,
                              RenderingComponent& rComp,
                              const RenderStagePass& renderStagePass,
                              const Camera& camera,
                              bool refreshData) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(rComp);
    ACKNOWLEDGE_UNUSED(renderStagePass);
    ACKNOWLEDGE_UNUSED(camera);
    ACKNOWLEDGE_UNUSED(refreshData);

    return getState() == ResourceState::RES_LOADED;
}

void SceneNode::postLoad(SceneGraphNode& sgn) {
    sgn.postLoad();
}

void SceneNode::setBounds(const BoundingBox& aabb) {
    _boundsChanged = true;
    _boundingBox.set(aabb);
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
                                 _materialTemplate->resourceName().c_str(),
                                 material->resourceName().c_str());
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

void SceneNode::editorFieldChanged(const char* field) {
    ACKNOWLEDGE_UNUSED(field);
}

void SceneNode::buildDrawCommands(SceneGraphNode& sgn,
                                  const RenderStagePass& renderStagePass,
                                  const Camera& crtCamera,
                                  RenderPackage& pkgInOut) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(renderStagePass);
    ACKNOWLEDGE_UNUSED(crtCamera);
    ACKNOWLEDGE_UNUSED(pkgInOut);
}

void SceneNode::onRefreshNodeData(const SceneGraphNode& sgn,
                                  const RenderStagePass& renderStagePass,
                                  const Camera& crtCamera,
                                  bool refreshData,
                                  GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(renderStagePass);
    ACKNOWLEDGE_UNUSED(crtCamera);
    ACKNOWLEDGE_UNUSED(refreshData);
    ACKNOWLEDGE_UNUSED(bufferInOut);
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
    ACKNOWLEDGE_UNUSED(outputBuffer);
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

};