#include "stdafx.h"

#include "Headers/SceneShaderData.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
SceneShaderData::SceneShaderData(GFXDevice& context)
    : _context(context),
      _dirty(true),
      _sceneShaderData(nullptr)
{
    shadowingSettings(0.25f, 0.00002f, 1000.0f, 1500.0f);

    ShaderBufferDescriptor bufferDescriptor = {};
    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._elementCount = 1;
    bufferDescriptor._elementSize = sizeof(SceneShaderBufferData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._initialData = { &_bufferData, bufferDescriptor._elementSize };
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
    bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
    // No need to use threaded writes as we manually update the buffer at the end of the frame. Let the API decide what's best based on data size
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::AUTO_RANGE_FLUSH);
    bufferDescriptor._name = "SCENE_SHADER_DATA";
    _sceneShaderData = _context.newSB(bufferDescriptor);
    _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
}

ShaderBuffer* SceneShaderData::uploadToGPU() {
    if (_dirty) {
        _sceneShaderData->writeData(&_bufferData);
        _dirty = false;
    }

    return _sceneShaderData;
}

} //namespace Divide