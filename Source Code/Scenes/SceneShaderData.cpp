#include "stdafx.h"

#include "Headers/SceneShaderData.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
SceneShaderData::SceneShaderData(GFXDevice& context)
    : _context(context),
      _sceneShaderData(nullptr)
{
    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._primitiveCount = 1;
    bufferDescriptor._primitiveSizeInBytes = sizeof(SceneShaderBufferData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._unbound = false;
    bufferDescriptor._initialData = &_bufferData;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;

    _sceneShaderData = _context.newSB(bufferDescriptor);
    _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
    shadowingSettings(0.0000002f, 0.0002f, 200.0f, 350.0f);
}

SceneShaderData::~SceneShaderData()
{
}

void SceneShaderData::uploadToGPU() {
    _sceneShaderData->writeData(&_bufferData);
}

}; //namespace Divide