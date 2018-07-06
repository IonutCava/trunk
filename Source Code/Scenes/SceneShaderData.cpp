#include "Headers/SceneShaderData.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
SceneShaderData::SceneShaderData()
    : _sceneShaderData(nullptr)
{

}

void SceneShaderData::init() {
    _sceneShaderData = GFX_DEVICE.newSB("sceneShaderData", 1, false, false, BufferUpdateFrequency::OFTEN);
    _sceneShaderData->create(1, sizeof(SceneShaderData));
    _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
    shadowingSettings(0.0000002f, 0.0002f, 150.0f, 250.0f);
}

SceneShaderData::~SceneShaderData()
{
    if (_sceneShaderData) {
        _sceneShaderData->destroy();
        MemoryManager::DELETE(_sceneShaderData);
    }
}

void SceneShaderData::uploadToGPU() {
    if (_sceneShaderData) {
        _sceneShaderData->setData(&_bufferData);
    }
}

}; //namespace Divide