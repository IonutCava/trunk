#include "Headers/SceneShaderData.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
SceneShaderData::SceneShaderData(GFXDevice& context)
    : _context(context),
      _sceneShaderData(nullptr)
{
    ShaderBufferParams params;
    params._primitiveCount = 1;
    params._primitiveSizeInBytes = sizeof(SceneShaderBufferData);
    params._ringBufferLength = 1;
    params._unbound = false;
    params._initialData = &_bufferData;
    params._updateFrequency = BufferUpdateFrequency::OFTEN;

    _sceneShaderData = _context.newSB(params);
    _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
    shadowingSettings(0.0000002f, 0.0002f, 200.0f, 350.0f);
}

SceneShaderData::~SceneShaderData()
{
}

void SceneShaderData::uploadToGPU() {
    _sceneShaderData->setData(&_bufferData);
    _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
}

}; //namespace Divide