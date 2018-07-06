#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Headers/glUniformBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "core.h"

glUniformBufferObject::glUniformBufferObject() : _UBOid(0)
{
	assert(glewIsSupported("GL_ARB_uniform_buffer_object"));
}

glUniformBufferObject::~glUniformBufferObject()
{
	if(_UBOid > 0) GLCheck(glDeleteBuffers(1, &_UBOid));
}

void glUniformBufferObject::Create(GLint bufferIndex, bool dynamic, bool stream){

    _usage = (dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW) : GL_STATIC_DRAW);

	GLint maxUniformIndex;
	GLCheck(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformIndex));
    CLAMP<GLint>(bufferIndex, 0, maxUniformIndex);
	GLCheck(glGenBuffers(1, &_UBOid)); // Generate the buffer
}
