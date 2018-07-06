#include "Headers/SceneNode.h"

#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

namespace Divide {

SceneNode::SceneNode(const SceneNodeType& type) : SceneNode("default", type)
{
}

SceneNode::SceneNode(const stringImpl& name, const SceneNodeType& type)
    : Resource(name),
      _materialTemplate(nullptr),
      _sgnParentCount(0),
      _type(type),
      _LODcount(1)  ///<Defaults to 1 LOD level
{
    _boundingBox.second = true;
}

SceneNode::~SceneNode()
{
}

void SceneNode::sceneUpdate(const U64 deltaTime,
                            SceneGraphNode& sgn,
                            SceneState& sceneState)
{
    assert(_sgnParentCount != 0);
}

void SceneNode::postLoad(SceneGraphNode& sgn) {
    if (_sgnParentCount > 0) {
        AddRef();
    }
    _sgnParentCount++;

    sgn.postLoad();
}

bool SceneNode::getDrawState(RenderStage currentStage) {
    return _renderState.getDrawState(currentStage);
}


Material* const SceneNode::getMaterialTpl() {
    // UpgradableReadLock ur_lock(_materialLock);
    if (_materialTemplate == nullptr && _renderState.useDefaultMaterial()) {
        // UpgradeToWriteLock uw_lock(ur_lock);
        _materialTemplate = CreateResource<Material>(
            ResourceDescriptor("defaultMaterial_" + getName()));
        _materialTemplate->setShadingMode(Material::ShadingMode::BLINN_PHONG);
        REGISTER_TRACKED_DEPENDENCY(_materialTemplate);
    }
    return _materialTemplate;
}

void SceneNode::setMaterialTpl(Material* const mat) {
    if (mat) {  // If we need to update the material
        // UpgradableReadLock ur_lock(_materialLock);
        if (_materialTemplate) {  // If we had an old material
            if (_materialTemplate->getGUID() !=
                mat->getGUID()) {  // if the old material isn't the same as the

                                   // new one
                Console::printfn(Locale::get(_ID("REPLACE_MATERIAL")),
                                 _materialTemplate->getName().c_str(),
                                 mat->getName().c_str());
                // UpgradeToWriteLock uw_lock(ur_lock);
                UNREGISTER_TRACKED_DEPENDENCY(_materialTemplate);
                RemoveResource(_materialTemplate);  // remove the old material
                // ur_lock.lock();
            }
        }
        // UpgradeToWriteLock uw_lock(ur_lock);
        _materialTemplate = mat;  // set the new material
        REGISTER_TRACKED_DEPENDENCY(_materialTemplate);
    } else {  // if we receive a null material, the we need to remove this
              // node's material
        // UpgradableReadLock ur_lock(_materialLock);
        if (_materialTemplate) {
            // UpgradeToWriteLock uw_lock(ur_lock);
            UNREGISTER_TRACKED_DEPENDENCY(_materialTemplate);
            RemoveResource(_materialTemplate);
        }
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