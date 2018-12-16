#include "stdafx.h"

#include "Headers/SceneShaderData.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
SceneShaderData::SceneShaderData(GFXDevice& context)
    : _context(context),
      _sceneShaderData(nullptr),
      _dirty(true)
{
    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._elementCount = 1;
    bufferDescriptor._elementSize = sizeof(SceneShaderBufferData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._initialData = &_bufferData;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;
    bufferDescriptor._flags = (to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE) |
                               to_U32(ShaderBuffer::Flags::AUTO_RANGE_FLUSH) |
                               to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES));
    bufferDescriptor._name = "SCENE_SHADER_DATA";
    _sceneShaderData = _context.newSB(bufferDescriptor);
    _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
    shadowingSettings(0.0000002f, 0.0002f, 200.0f, 350.0f);
}

SceneShaderData::~SceneShaderData()
{
}

void SceneShaderData::uploadToGPU() {
    if (_dirty) {
        _sceneShaderData->writeData(&_bufferData);
        _dirty = false;
    }
}

}; //namespace Divide