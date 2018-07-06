#include "Headers/SceneNode.h"

#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

namespace {
    bool isRigidBody(SceneNode& node) {
        STUBBED("Due to the way that physics libs work in world transforms only, a classical scene graph type system does not "
            "fit the physics model well. Thus, for now, only first-tier nodes are physically simulated!");
        // If nodes need more complex physical relationships, they can be added as first tier nodes and linked together
        // with a joint type system and the "parent" node's 'relative' mass set to infinite so the child node couldn't move it
        if (node.getType() == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType crtType = static_cast<Object3D&>(node).getObjectType();
            if (crtType != Object3D::ObjectType::TEXT_3D &&
                crtType != Object3D::ObjectType::SUBMESH &&
                crtType != Object3D::ObjectType::FLYWEIGHT) {
                return true;
            }
        }
        return false;
    }

};

SceneNode::SceneNode(const stringImpl& name, const SceneNodeType& type)
    : SceneNode(name, "", type)
{
}

SceneNode::SceneNode(const stringImpl& name, const stringImpl& resourceLocation, const SceneNodeType& type)
    : Resource(name, resourceLocation),
     _materialTemplate(nullptr),
     _type(type),
     _LODcount(1)  ///<Defaults to 1 LOD level
{
}

SceneNode::~SceneNode()
{
}

void SceneNode::sceneUpdate(const U64 deltaTime,
                            SceneGraphNode& sgn,
                            SceneState& sceneState)
{
    vectorImpl<SceneNode::SGNParentData>::iterator it;
    it = getSGNData(sgn.getGUID());
    assert(it != std::cend(_sgnParents));
    SGNParentData& parentData = *it;

    if (parentData.getFlag(UpdateFlag::BOUNDS_CHANGED)) {
        updateBoundsInternal(sgn);
        BoundsComponent* bComp = sgn.get<BoundsComponent>();
        if (bComp) {
            bComp->onBoundsChange(_boundingBox);
        }
        parentData.clearFlag(UpdateFlag::BOUNDS_CHANGED);
    }
}

void SceneNode::updateBoundsInternal(SceneGraphNode& sgn) {
}

void SceneNode::postLoad(SceneGraphNode& sgn) {
    updateBoundsInternal(sgn);
    sgn.get<BoundsComponent>()->onBoundsChange(_boundingBox);
    sgn.postLoad();
}

bool SceneNode::getDrawState(RenderStage currentStage) {
    return _renderState.getDrawState(currentStage);
}

const Material_ptr& SceneNode::getMaterialTpl() {
    // UpgradableReadLock ur_lock(_materialLock);
    if (_materialTemplate == nullptr && _renderState.useDefaultMaterial()) {
        // UpgradeToWriteLock uw_lock(ur_lock);
        _materialTemplate = CreateResource<Material>(ResourceDescriptor("defaultMaterial_" + getName()));
        _materialTemplate->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    }
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
                                 _materialTemplate->getName().c_str(),
                                 material->getName().c_str());
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

bool SceneNode::getDrawCommands(SceneGraphNode& sgn,
                                RenderStage renderStage,
                                const SceneRenderState& sceneRenderState,
                                vectorImpl<GenericDrawCommand>& drawCommandsOut) {
    return true;
}

void SceneNode::postRender(SceneGraphNode& sgn) const {
}

};