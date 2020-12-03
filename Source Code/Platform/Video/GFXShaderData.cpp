#include "stdafx.h"

#include "Headers/GFXShaderData.h"

namespace Divide {

GFXShaderData::GPUData::GPUData() noexcept
{
    _ProjectionMatrix.identity();
    _InvProjectionMatrix.identity();
    _ViewMatrix.identity();
    _InvViewMatrix.identity();
    _ViewProjectionMatrix.identity();
    _PreviousViewProjectionMatrix.identity();
    _cameraPosition.set(0.0f);
    _ViewPort.set(1.0f);
    _renderProperties.set(0.01f, 100.0f, 1.0f, 0.0f);
    _clipPlanes.fill({ 0.0f, 0.0f, 0.0f, 0.0f });
    _frustumPlanes.fill({0.0f, 0.0f, 0.0f, 0.0f});

}

}; //namespace Divide