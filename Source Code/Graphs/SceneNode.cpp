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
      _hasSGNParent(false),
      _type(type),
      _LODcount(1)  ///<Defaults to 1 LOD level
{
}

SceneNode::~SceneNode() {}

void SceneNode::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                            SceneState& sceneState) {}

bool SceneNode::getDrawState(RenderStage currentStage) {
    return _renderState.getDrawState(currentStage);
}

bool SceneNode::getDrawCommands(SceneGraphNode& sgn,
                                RenderStage renderStage,
                                const SceneRenderState& sceneRenderState,
                                vectorImpl<GenericDrawCommand>& drawCommandsOut) {
    return true;
}

bool SceneNode::isInView(const SceneRenderState& sceneRenderState,
                         const SceneGraphNode& sgn,
                         Frustum::FrustCollision& collisionType,
                         bool distanceCheck) const {
    const BoundingBox& boundingBox = sgn.getBoundingBoxConst();
    const BoundingSphere& sphere = sgn.getBoundingSphereConst();

    const Camera& cam = sceneRenderState.getCameraConst();
    const vec3<F32>& eye = cam.getEye();
    const Frustum& frust = cam.getFrustumConst();
    F32 radius = sphere.getRadius();
    const vec3<F32>& center = sphere.getCenter();
    F32 cameraDistance = center.distance(eye);
    F32 visibilityDistance = GET_ACTIVE_SCENE().state().generalVisibility() + radius;

    if (distanceCheck && cameraDistance > visibilityDistance) {
        if (boundingBox.nearestDistanceFromPointSquared(eye) >
            std::min(visibilityDistance, sceneRenderState.getCameraConst().getZPlanes().y)) {
            return false;
        }
    }

    if (!boundingBox.containsPoint(eye)) {
        collisionType = frust.ContainsSphere(center, radius);
        switch (collisionType) {
            case Frustum::FrustCollision::FRUSTUM_OUT: {
                return false;
            };
            case Frustum::FrustCollision::FRUSTUM_INTERSECT: {
                collisionType = frust.ContainsBoundingBox(boundingBox);
                if (collisionType == Frustum::FrustCollision::FRUSTUM_OUT) {
                    return false;
                }
            };
        }
    }

    return true;
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

bool SceneNode::computeBoundingBox(SceneGraphNode& sgn) {
    sgn.setInitialBoundingBox(sgn.getBoundingBoxConst());
    sgn.getBoundingBox().setComputed(true);
    return true;
}

bool SceneNode::unload() {
    setMaterialTpl(nullptr);
    return true;
}

void SceneNode::postDrawBoundingBox(SceneGraphNode& sgn) const {
}

};