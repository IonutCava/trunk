#include "stdafx.h"

#include "Headers/glIMPrimitive.h"
#include "Headers/glResources.h"
#include "Headers/GLWrapper.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include <GLIM/glim.h>

namespace Divide {

glIMPrimitive::glIMPrimitive(GFXDevice& context)
    : IMPrimitive(context)
{
    _imInterface = MemoryManager_NEW NS_GLIM::GLIM_BATCH();
    _imInterface->SetVertexAttribLocation(to_base(AttribLocation::VERTEX_POSITION));
}

glIMPrimitive::~glIMPrimitive()
{
    MemoryManager::DELETE(_imInterface);
}

void glIMPrimitive::beginBatch(bool reserveBuffers, U32 vertexCount, U32 attributeCount) {
    _imInterface->BeginBatch(reserveBuffers, vertexCount, attributeCount);
}

void glIMPrimitive::begin(PrimitiveType type) {
    _imInterface->Begin(GLUtil::glimPrimitiveType[to_U32(type)]);
}

void glIMPrimitive::vertex(F32 x, F32 y, F32 z) {
    _imInterface->Vertex(x, y, z);
}

void glIMPrimitive::attribute1f(U32 attribLocation, F32 value) {
    _imInterface->Attribute1f(attribLocation, value);
}

void glIMPrimitive::attribute4ub(U32 attribLocation, U8 x, U8 y, U8 z, U8 w) {
    _imInterface->Attribute4ub(attribLocation, x, y, z, w);
}

void glIMPrimitive::attribute4f(U32 attribLocation, F32 x, F32 y, F32 z, F32 w) {
    _imInterface->Attribute4f(attribLocation, x, y, z, w);
}

void glIMPrimitive::attribute1i(U32 attribLocation, I32 value) {
    _imInterface->Attribute1i(attribLocation, value);
}

void glIMPrimitive::end() {
    _imInterface->End();
}

void glIMPrimitive::endBatch() {
    _imInterface->EndBatch();
}

void glIMPrimitive::pipeline(const Pipeline& pipeline) {

    IMPrimitive::pipeline(pipeline);
    _imInterface->SetShaderProgramHandle(pipeline.shaderProgramHandle());
}

void glIMPrimitive::draw(const GenericDrawCommand& cmd) {
    _imInterface->RenderBatchInstanced(cmd._cmd.primCount, 
                                       _forceWireframe || isEnabledOption(cmd, CmdRenderOptions::RENDER_WIREFRAME));
}

GFX::CommandBuffer& glIMPrimitive::toCommandBuffer() const {
    if (_cmdBufferDirty) {
        _cmdBuffer->clear();

        DIVIDE_ASSERT(_pipeline->shaderProgramHandle() != 0,
                      "glIMPrimitive error: Draw call received without a valid shader defined!");

        GenericDrawCommand cmd;
        cmd._sourceBuffer = const_cast<glIMPrimitive*>(this);

        PushConstants pushConstants;
        // Inform the shader if we have (or don't have) a texture
        pushConstants.set("useTexture", GFX::PushConstantType::BOOL, _texture != nullptr);
        // Upload the primitive's world matrix to the shader
        pushConstants.set("dvd_WorldMatrix", GFX::PushConstantType::MAT4, worldMatrix());

        GFX::BindPipelineCommand pipelineCommand;
        pipelineCommand._pipeline = _pipeline;
        GFX::EnqueueCommand(*_cmdBuffer, pipelineCommand);
        
        GFX::SendPushConstantsCommand pushConstantsCommand;
        pushConstantsCommand._constants = pushConstants;
        GFX::EnqueueCommand(*_cmdBuffer, pushConstantsCommand);

        if (_texture) {
            GFX::BindDescriptorSetsCommand descriptorSetCmd;
            descriptorSetCmd._set = _descriptorSet;
            GFX::EnqueueCommand(*_cmdBuffer, descriptorSetCmd);
        }

        if (_viewport != vec4<I32>(-1)) {
            GFX::SetViewportCommand setViewportCmd;
            setViewportCmd._viewport = _viewport;
            GFX::EnqueueCommand(*_cmdBuffer, setViewportCmd);
        }

        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(cmd);
        GFX::EnqueueCommand(*_cmdBuffer, drawCommand);

        _cmdBufferDirty = false;
    }

    return *_cmdBuffer;
}

};