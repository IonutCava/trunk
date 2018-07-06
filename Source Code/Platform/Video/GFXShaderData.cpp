#include "Headers/GFXShaderData.h"

namespace Divide {

GFXShaderData::GFXShaderData()
    : _needsUpload(true),
      _data(GPUData())
{
}

GFXShaderData::GPUData::GPUData()
{
    _ProjectionMatrix.identity();
    _InvProjectionMatrix.identity();
    _ViewMatrix.identity();
    _ViewProjectionMatrix.identity();
    _cameraPosition.set(0.0f);
    _ViewPort.set(1.0f);
    _ZPlanesCombined.set(1.0f, 1.1f, 1.0f, 1.1f);
    _invScreenDimension.set(1.0f);
    _renderProperties.set(0.0f);
    for (U8 i = 0; i < to_const_ubyte(Frustum::FrustPlane::COUNT); ++i) {
        _frustumPlanes[i].set(0.0f);
        _clipPlanes[i].set(1.0f);
    }
}

}; //namespace Divide