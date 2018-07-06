#include "Headers/SceneShaderData.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
SceneShaderData::SceneShaderData(GFXDevice& context)
    : _context(context),
      _sceneShaderData(nullptr)
{
    _sceneShaderData = _context.newSB(1, false, false, BufferUpdateFrequency::OFTEN);
    _sceneShaderData->create(1, sizeof(SceneShaderData));
    _sceneShaderData->bind(ShaderBufferLocation::SCENE_DATA);
    shadowingSettings(0.0000002f, 0.0002f, 200.0f, 350.0f);
}

SceneShaderData::~SceneShaderData()
{
}

void SceneShaderData::uploadToGPU() {
    _sceneShaderData->setData(&_bufferData);
}

}; //namespace Divide