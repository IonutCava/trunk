#include "stdafx.h"

#include "Headers/Camera.h"

namespace Divide {

Camera::Camera(const Str256& name, const CameraType& type, const vec3<F32>& eye)
    : Resource(ResourceType::DEFAULT, name),
      _type(type)
{
    _data._eye.set(eye);
    _data._FoV = 60.0f;
    _data._aspectRatio = 1.77f;
    _data._viewMatrix.identity();
    _data._projectionMatrix.identity();
    _data._zPlanes.set(0.1f, 1.0f);
    _data._orientation.identity();
}

const CameraSnapshot& Camera::snapshot() const noexcept {
    assert(!dirty());
    return _data;
}

void Camera::fromCamera(const Camera& camera, bool flag) {
    ACKNOWLEDGE_UNUSED(flag);

    _reflectionPlane = camera._reflectionPlane;
    _reflectionActive = camera._reflectionActive;
    _accumPitchDegrees = camera._accumPitchDegrees;

    lockFrustum(camera._frustumLocked);
    if (camera._data._isOrthoCamera) {
        setAspectRatio(camera.getAspectRatio());
        setVerticalFoV(camera.getVerticalFoV());

        setProjection(camera._orthoRect, camera.getZPlanes());
    } else {
        _orthoRect.set(camera._orthoRect);

        setProjection(camera.getAspectRatio(), camera.getVerticalFoV(), camera.getZPlanes());
    }

    setEye(camera.getEye());
    setRotation(camera._data._orientation);
    updateLookAt();
}

void Camera::fromSnapshot(const CameraSnapshot& snapshot) {
    setEye(snapshot._eye);
    setRotation(snapshot._orientation);
    setAspectRatio(snapshot._aspectRatio);
    if (_data._isOrthoCamera) {
        setProjection(_orthoRect, snapshot._zPlanes);
    } else {
        setProjection(snapshot._aspectRatio, snapshot._FoV, snapshot._zPlanes);
    }
    updateLookAt();
}

void Camera::update(const F32 deltaTimeMS) noexcept {
    ACKNOWLEDGE_UNUSED(deltaTimeMS);

    NOP();
}

vec3<F32> ExtractCameraPos2(const mat4<F32>& a_modelView)
{
    // Get the 3 basis vector planes at the camera origin and transform them into model space.
    //  
    // NOTE: Planes have to be transformed by the inverse transpose of a matrix
    //       Nice reference here: http://www.opengl.org/discussion_boards/showthread.php/159564-Clever-way-to-transform-plane-by-matrix
    //
    //       So for a transform to model space we need to do:
    //            inverse(transpose(inverse(MV)))
    //       This equals : transpose(MV) - see Lemma 5 in http://mathrefresher.blogspot.com.au/2007/06/transpose-of-matrix.html
    //
    // As each plane is simply (1,0,0,0), (0,1,0,0), (0,0,1,0) we can pull the data directly from the transpose matrix.
    //  
    const mat4<F32> modelViewT(a_modelView.getTranspose());

    // Get plane normals 
    const vec4<F32> n1(modelViewT.getRow(0));
    const vec4<F32> n2(modelViewT.getRow(1));
    const vec4<F32> n3(modelViewT.getRow(2));

    // Get plane distances
    const F32 d1(n1.w);
    const F32 d2(n2.w);
    const F32 d3(n3.w);

    // Get the intersection of these 3 planes 
    // (uisng math from RealTime Collision Detection by Christer Ericson)
    const vec3<F32> n2n3 = Cross(n2.xyz(), n3.xyz());
    const float denom = Dot(n1.xyz(), n2n3);
    const vec3<F32> top = (n2n3 * d1) + Cross(n1.xyz(), (d3*n2.xyz()) - (d2*n3.xyz()));
    return top / -denom;
}
const mat4<F32>& Camera::lookAt(const mat4<F32>& viewMatrix) {
    _data._eye.set(ExtractCameraPos2(viewMatrix));
    _data._orientation.fromMatrix(viewMatrix);
    _viewMatrixDirty = true;
    _frustumDirty = true;
    updateViewMatrix();

    return getViewMatrix();
}

const mat4<F32>& Camera::lookAt(const vec3<F32>& eye,
                                const vec3<F32>& target,
                                const vec3<F32>& up) {
    _data._eye.set(eye);
    _data._orientation.fromMatrix(mat4<F32>(eye, target, up));
    _viewMatrixDirty = true;
    _frustumDirty = true;

    updateViewMatrix();

    return getViewMatrix();
}

/// Tell the rendering API to set up our desired PoV
bool Camera::updateLookAt() {
    OPTICK_EVENT();

    bool cameraUpdated =  updateViewMatrix();
    cameraUpdated = updateProjection() || cameraUpdated;
    cameraUpdated = updateFrustum() || cameraUpdated;
    
    if (cameraUpdated) {
        for (ListenerMap::value_type it : _updateCameraListeners) {
            it.second(*this);
        }
    }

    return cameraUpdated;
}

void Camera::setGlobalRotation(F32 yaw, F32 pitch, F32 roll) {
    const Quaternion<F32> pitchRot(WORLD_X_AXIS, -pitch);
    const Quaternion<F32> yawRot(WORLD_Y_AXIS, -yaw);

    if (!IS_ZERO(roll)) {
        setRotation(yawRot * pitchRot * Quaternion<F32>(WORLD_Z_AXIS, -roll));
    } else {
        setRotation(yawRot * pitchRot);
    }
}

bool Camera::removeUpdateListener(U32 id) {
    const auto& it = _updateCameraListeners.find(id);
    if (it != std::cend(_updateCameraListeners)) {
        _updateCameraListeners.erase(it);
        return true;
    }

    return false;
}

U32 Camera::addUpdateListener(const DELEGATE<void, const Camera&>& f) {
    hashAlg::insert(_updateCameraListeners, ++_updateCameraId, f);
    return _updateCameraId;
}

void Camera::setReflection(const Plane<F32>& reflectionPlane) {
    _reflectionPlane = reflectionPlane;
    _reflectionActive = true;
    _viewMatrixDirty = true;
}

void Camera::clearReflection() noexcept {
    _reflectionActive = false;
    _viewMatrixDirty = true;
}

bool Camera::updateProjection() {
    if (_projectionDirty) {
        if (_data._isOrthoCamera) {
            _data._projectionMatrix.ortho(_orthoRect.left,
                                          _orthoRect.right,
                                          _orthoRect.bottom,
                                          _orthoRect.top,
                                          _data._zPlanes.x,
                                          _data._zPlanes.y);
        } else {
            _data._projectionMatrix.perspective(_data._FoV,
                                                _data._aspectRatio,
                                                _data._zPlanes.x,
                                                _data._zPlanes.y);
        }

        _frustumDirty = true;
        _projectionDirty = false;
        return true;
    }

    return false;
}

const mat4<F32>& Camera::setProjection(const vec2<F32>& zPlanes) {
    return setProjection(_data._FoV, zPlanes);
}

const mat4<F32>& Camera::setProjection(F32 verticalFoV, const vec2<F32>& zPlanes) {
    return setProjection(_data._aspectRatio, verticalFoV, zPlanes);
}

const mat4<F32>& Camera::setProjection(F32 aspectRatio, F32 verticalFoV, const vec2<F32>& zPlanes) {
    setAspectRatio(aspectRatio);
    setVerticalFoV(verticalFoV);

    _data._zPlanes = zPlanes;
    _data._isOrthoCamera = false;
    _projectionDirty = true;
    updateProjection();

    return getProjectionMatrix();
}

const mat4<F32>& Camera::setProjection(const vec4<F32>& rect, const vec2<F32>& zPlanes) {
    _data._zPlanes = zPlanes;
    _orthoRect = rect;
    _data._isOrthoCamera = true;
    _projectionDirty = true;
    updateProjection();

    return getProjectionMatrix();
}

const mat4<F32>& Camera::setProjection(const mat4<F32>& projection, const vec2<F32>& zPlanes, bool isOrtho) noexcept {
    _data._projectionMatrix.set(projection);
    _data._zPlanes = zPlanes;
    _projectionDirty = false;
    _frustumDirty = true;
    _data._isOrthoCamera = isOrtho;

    return _data._projectionMatrix;
}

void Camera::setAspectRatio(F32 ratio) noexcept {
    _data._aspectRatio = ratio;
    _projectionDirty = true;
}

void Camera::setVerticalFoV(Angle::DEGREES<F32> verticalFoV) noexcept {
    _data._FoV = verticalFoV;
    _projectionDirty = true;
}

void Camera::setHorizontalFoV(Angle::DEGREES<F32> horizontalFoV) noexcept {
    _data._FoV = Angle::to_VerticalFoV(horizontalFoV, to_D64(_data._aspectRatio));
    _projectionDirty = true;
}

bool Camera::updateViewMatrix() {
    if (!_viewMatrixDirty) {
        return false;
    }

    _data._orientation.normalize();

    //_target = -zAxis + _data._eye;

    // Reconstruct the view matrix.
    _data._viewMatrix.set(GetMatrix(_data._orientation));
    _data._viewMatrix.setRow(3, -_data._orientation.xAxis().dot(_data._eye),
                                -_data._orientation.yAxis().dot(_data._eye),
                                -_data._orientation.zAxis().dot(_data._eye), 
                                1.0f);
    _data._orientation.getEuler(_euler);
    _euler = Angle::to_DEGREES(_euler);
    // Extract the pitch angle from the view matrix.
    _accumPitchDegrees = Angle::to_DEGREES(std::asinf(getForwardDir().y));

    if (_reflectionActive) {
        _data._viewMatrix.reflect(_reflectionPlane);
        _data._eye.set(mat4<F32>(_reflectionPlane).transformNonHomogeneous(_data._eye));
    }

    _viewMatrixDirty = false;
    _frustumDirty = true;

    return true;
}

bool Camera::updateFrustum() {
    if (_frustumLocked) {
        return true;
    }
    if (!_frustumDirty) {
        return false;
    }

    _frustum.Extract(getViewMatrix(), getProjectionMatrix());
    _frustumDirty = false;

    return true;
}

vec3<F32> Camera::unProject(F32 winCoordsX, F32 winCoordsY, F32 winCoordsZ, const Rect<I32>& viewport) const {
    Rect<F32> temp = {
        (winCoordsX - F32(viewport[0])) / F32(viewport[2]),
        (winCoordsY - F32(viewport[1])) / F32(viewport[3]),
        winCoordsZ,
        1.0f
    };

    temp = (getViewMatrix() * getProjectionMatrix()).getInverse() * (2.0f * temp - 1.0f);
    return (temp.xyz() / temp.w);
}

vec2<F32> Camera::project(const vec3<F32>& worldCoords, const Rect<I32>& viewport) const {
    const vec4<F32> clipSpace = getProjectionMatrix() * (getViewMatrix() * vec4<F32>(worldCoords, 1.0f));
    const vec3<F32> ndcSpace = clipSpace.xyz() / std::max(clipSpace.w, std::numeric_limits<F32>::epsilon());
    const vec2<F32> winSpace = ((ndcSpace.xy() + 1.0f) * 0.5f) * viewport.zw() + viewport.xy();
    return winSpace;
}

};
