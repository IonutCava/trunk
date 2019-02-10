#include "stdafx.h"

#include "Headers/GFXShaderData.h"

namespace Divide {

GFXShaderData::GFXShaderData() noexcept
    : _needsUpload(true)
{
}

GFXShaderData::GPUData::GPUData() noexcept
{
    _ProjectionMatrix.identity();
    _ViewMatrix.identity();
    _ViewProjectionMatrix.identity();
    _cameraPosition.set(0.0f);
    _ViewPort.set(1.0f);
    _renderProperties.set(0.01f, 100.0f, 1.0f, 0.0f);
    for (U8 i = 0; i < to_base(Frustum::FrustPlane::COUNT); ++i) {
        _clipPlanes[i].set(0.0f);
    }
}

bool GFXShaderData::GPUData::operator==(const GPUData& other) const {
    for (U32 i = 0; i < to_base(Frustum::FrustPlane::COUNT); ++i) {
        if (_clipPlanes[i] != other._clipPlanes[i]) {
            return false;
        }
    }

    return _cameraPosition == other._cameraPosition &&
           _ViewPort == other._ViewPort &&
           _renderProperties == other._renderProperties &&
           _ProjectionMatrix == other._ProjectionMatrix &&
           _ViewMatrix == other._ViewMatrix &&
           _ViewProjectionMatrix == other._ViewProjectionMatrix;
}

bool GFXShaderData::GPUData::operator!=(const GPUData& other) const {
    for (U32 i = 0; i < to_base(Frustum::FrustPlane::COUNT); ++i) {
        if (_clipPlanes[i] != other._clipPlanes[i]) {
            return true;
        }
    }

    return _cameraPosition != other._cameraPosition ||
           _ViewPort != other._ViewPort ||
           _renderProperties != other._renderProperties ||
           _ProjectionMatrix != other._ProjectionMatrix ||
           _ViewMatrix != other._ViewMatrix ||
           _ViewProjectionMatrix != other._ViewProjectionMatrix;
}

}; //namespace Divide