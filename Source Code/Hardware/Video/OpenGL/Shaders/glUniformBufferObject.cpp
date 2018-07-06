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
	GLCheck(glDeleteBuffers(1, &_UBOid));
}

void glUniformBufferObject::Create(GLushort dataSize, GLint bufferIndex){

	GLint maxUniformIndex;
	GLCheck(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformIndex));
    CLAMP<GLint>(bufferIndex, 0, maxUniformIndex);
	GLCheck(glGenBuffers(1, &_UBOid)); // Generate the buffer
	GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, _UBOid));
	GLCheck(glBufferData(GL_UNIFORM_BUFFER, sizeof(GLfloat) * dataSize, NULL, GL_STREAM_DRAW));
	GLCheck(glBindBufferRange(GL_UNIFORM_BUFFER, bufferIndex, _UBOid, 0, sizeof(GLfloat) * dataSize));
}

void glUniformBufferObject::FillData(GLint size, void* data){
	GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, _UBOid));
	GLCheck(glBufferSubData(GL_UNIFORM_BUFFER, 0, (sizeof(GLfloat) * 8 * size), data));
	GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}