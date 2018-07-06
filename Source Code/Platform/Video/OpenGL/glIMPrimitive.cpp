#include "Headers/glIMPrimitive.h"
#include "Headers/glResources.h"
#include "Headers/GLWrapper.h"

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
    if (IMPrimitive::drawShader() == nullptr) {
        drawShader(ShaderProgram::defaultShader());
    }
}

void glIMPrimitive::drawShader(const ShaderProgram_ptr& shaderProgram) {
    if (shaderProgram == nullptr) {
        // Inform the primitive that we're using the imShader
        // A primitive can be rendered with any shader program available, so
        // make sure we actually use the right one for this stage
        drawShader(ShaderProgram::defaultShader());
    } else {
        IMPrimitive::drawShader(shaderProgram);
        _imInterface->SetShaderProgramHandle(_drawShader->getID());
    }
}

void glIMPrimitive::draw(const GenericDrawCommand& command) {
    if (paused() || command.drawCount() == 0) {
        return;
    }

    setupStates();

    // Check if any texture is present
    bool texture = (_texture != nullptr);
    // And bind it to the first diffuse texture slot
    if (texture) {
        _texture->bind(to_U8(ShaderProgram::TextureUsage::UNIT0));
    }

    // Inform the shader if we have (or don't have) a texture
    command.shaderProgram()->Uniform("useTexture", texture);
    // Upload the primitive's world matrix to the shader
    command.shaderProgram()->Uniform("dvd_WorldMatrix", worldMatrix());

    GLuint query = 0;
    bool queryPrimitives = command.isEnabledOption(GenericDrawCommand::RenderOptions::QUERY_PRIMITIVE_COUNT);

    if (queryPrimitives) {
        glGenQueries(1, &query);
        glBeginQuery(GL_PRIMITIVES_GENERATED, query);
    }

    _imInterface->RenderBatchInstanced(command.cmd().primCount,
                                       command.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME));

    if (queryPrimitives) {
        glEndQuery(GL_PRIMITIVES_GENERATED);
        glGetQueryObjectiv(query, GL_QUERY_RESULT, &_primitivesGenerated);
    }

    // Call any "postRender" function the primitive may have attached
    resetStates();
}

GenericDrawCommand glIMPrimitive::toDrawCommand() const {
    GenericDrawCommand cmd;
    if (!paused()) {
        DIVIDE_ASSERT(_drawShader != nullptr,
                      "glIMPrimitive error: Draw call received without a valid shader defined!");

        cmd.sourceBuffer(const_cast<glIMPrimitive*>(this));
        cmd.shaderProgram(_drawShader);
        cmd.stateHash(stateHash());
    } else {
        cmd.drawCount(0);
    }

    return cmd;
}

};