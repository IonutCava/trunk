#include "stdafx.h"

#include "Headers/SceneShaderData.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
SceneShaderData::SceneShaderData(GFXDevice& context)
    : _context(context)
{
    shadowingSettings(0.25f, 0.00002f, 1000.0f, 1500.0f);

    ShaderBufferDescriptor bufferDescriptor = {};
    bufferDescriptor._usage = ShaderBuffer::Usage::CONSTANT_BUFFER;
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
    // No need to use threaded writes as we manually update the buffer at the end of the frame. Let the API decide what's best based on data size
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::AUTO_RANGE_FLUSH);
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;
    {
        bufferDescriptor._elementCount = 1;
        bufferDescriptor._elementSize = sizeof(SceneShaderBufferData);
        bufferDescriptor._initialData = { (Byte*)&_sceneBufferData, bufferDescriptor._elementSize };
        bufferDescriptor._name = "SCENE_SHADER_DATA";
        _sceneShaderData = _context.newSB(bufferDescriptor);
        _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
    }
    {
        bufferDescriptor._elementCount = GLOBAL_PROBE_COUNT;
        bufferDescriptor._elementSize = sizeof(ProbeData);
        bufferDescriptor._initialData = { (Byte*)_probeData.data(), bufferDescriptor._elementSize };
        bufferDescriptor._name = "SCENE_PROBE_DATA";
        _probeShaderData = _context.newSB(bufferDescriptor);
        _probeShaderData->bind(ShaderBufferLocation::PROBE_DATA);
    }
}

void SceneShaderData::uploadToGPU() {
    if (_sceneDataDirty) {
        _sceneShaderData->writeData(&_sceneBufferData);
        _sceneDataDirty = false;
    }

    if (_probeDataDirty) {
        _probeShaderData->writeData(_probeData.data());
        _probeDataDirty = false;
    }
}

} //namespace Divide