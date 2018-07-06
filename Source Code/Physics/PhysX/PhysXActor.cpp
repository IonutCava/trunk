#include "Headers/PhysXActor.h"

namespace Divide {
    PhysXActor::PhysXActor(PhysicsComponent& parent)
        : PhysicsAsset(parent),
        _actor(nullptr),
        _isDynamic(false),
        _userData(0.0f)
    {
    }

    PhysXActor::~PhysXActor() {}

    /// Set the local X,Y and Z position
    void PhysXActor::setPosition(const vec3<F32>& position) {
    }

    /// Set the object's position on the X axis
    void PhysXActor::setPositionX(const F32 positionX) {

    }
    /// Set the object's position on the Y axis
    void PhysXActor::setPositionY(const F32 positionY) {

    }
    /// Set the object's position on the Z axis
    void PhysXActor::setPositionZ(const F32 positionZ) {

    }

    /// Set the local X,Y and Z scale factors
    void PhysXActor::setScale(const vec3<F32>& scale) {

    }

    /// Set the local orientation using the Axis-Angle system.
    /// The angle can be in either degrees(default) or radians
    void PhysXActor::setRotation(const vec3<F32>& axis, F32 degrees, bool inDegrees) {

    }

    /// Set the local orientation using the Euler system.
    /// The angles can be in either degrees(default) or radians
    void PhysXActor::setRotation(F32 pitch, F32 yaw, F32 roll, bool inDegrees) {

    }

    /// Set the local orientation so that it matches the specified quaternion.
    void PhysXActor::setRotation(const Quaternion<F32>& quat) {

    }

    /// Add the specified translation factors to the current local position
    void PhysXActor::translate(const vec3<F32>& axisFactors) {

    }

    /// Add the specified scale factors to the current local position
    void PhysXActor::scale(const vec3<F32>& axisFactors) {

    }

    /// Apply the specified Axis-Angle rotation starting from the current
    /// orientation.
    /// The angles can be in either degrees(default) or radians
    void PhysXActor::rotate(const vec3<F32>& axis, F32 degrees, bool inDegrees) {

    }

    /// Apply the specified Euler rotation starting from the current
    /// orientation.
    /// The angles can be in either degrees(default) or radians
    void PhysXActor::rotate(F32 pitch, F32 yaw, F32 roll, bool inDegrees) {

    }

    /// Apply the specified Quaternion rotation starting from the current orientation.
    void PhysXActor::rotate(const Quaternion<F32>& quat) {

    }

    /// Perform a SLERP rotation towards the specified quaternion
    void PhysXActor::rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) {

    }

    /// Set the scaling factor on the X axis
    void PhysXActor::setScaleX(const F32 ammount) {

    }

    /// Set the scaling factor on the Y axis
    void PhysXActor::setScaleY(const F32 ammount) {

    }

    /// Set the scaling factor on the Z axis
    void PhysXActor::setScaleZ(const F32 ammount) {

    }

    /// Increase the scaling factor on the X axis by the specified factor
    void PhysXActor::scaleX(const F32 ammount) {

    }

    /// Increase the scaling factor on the Y axis by the specified factor
    void PhysXActor::scaleY(const F32 ammount) {

    }

    /// Increase the scaling factor on the Z axis by the specified factor
    void PhysXActor::scaleZ(const F32 ammount) {

    }

    /// Rotate on the X axis (Axis-Angle used) by the specified angle (either
    /// degrees or radians)
    void PhysXActor::rotateX(const F32 angle, bool inDegrees) {

    }

    /// Rotate on the Y axis (Axis-Angle used) by the specified angle (either
    /// degrees or radians)
    void PhysXActor::rotateY(const F32 angle, bool inDegrees) {

    }

    /// Rotate on the Z axis (Axis-Angle used) by the specified angle (either
    /// degrees or radians)
    void PhysXActor::rotateZ(const F32 angle, bool inDegrees) {

    }

    /// Set the rotation on the X axis (Axis-Angle used) by the specified angle
    /// (either degrees or radians)
    void PhysXActor::setRotationX(const F32 angle, bool inDegrees) {

    }

    /// Set the rotation on the Y axis (Axis-Angle used) by the specified angle
    /// (either degrees or radians)
    void PhysXActor::setRotationY(const F32 angle, bool inDegrees) {

    }

    /// Set the rotation on the Z axis (Axis-Angle used) by the specified angle
    /// (either degrees or radians)
    void PhysXActor::setRotationZ(const F32 angle, bool inDegrees) {

    }

    /// Return the scale factor
    vec3<F32> PhysXActor::getScale() const {
        return vec3<F32>(1.0f);
    }

    /// Return the position
    vec3<F32> PhysXActor::getPosition() const {
        return vec3<F32>(0.0f);
    }
    /// Return the orientation quaternion
    Quaternion<F32> PhysXActor::getOrientation() const {
        return Quaternion<F32>();
    }

    const mat4<F32>& PhysXActor::getMatrix() {
        return _cachedLocalMatrix;
    }
}; //namespace Divide