#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Headers/glUniformBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "core.h"

vectorImpl<GLuint> glUniformBufferObject::_bindingIndices;

GLuint glUniformBufferObject::getBindingIndice() {
	for(GLuint i = 0, size = _bindingIndices.size(); i < size; i++) {
        if(_bindingIndices[i] != i) {
            _bindingIndices[i] = i;
            return i;
        }
    }
    _bindingIndices.push_back(_bindingIndices.size());
    return _bindingIndices.size();
}

glUniformBufferObject::glUniformBufferObject() : GUIDWrapper(), _UBOid(0), _bindIndex(0)
{
	assert(glewIsSupported("GL_ARB_uniform_buffer_object"));
}

glUniformBufferObject::~glUniformBufferObject()
{
	if(_UBOid > 0) GLCheck(glDeleteBuffers(1, &_UBOid));
}

void glUniformBufferObject::Create(GLint bufferIndex, bool dynamic, bool stream){
    _usage = (dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW) : GL_STATIC_DRAW);
	_bindIndex = getBindingIndice();
	GLint maxUniformIndex;
	GLCheck(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformIndex));
    CLAMP<GLuint>(_bindIndex, 0, maxUniformIndex);
	GLCheck(glGenBuffers(1, &_UBOid)); // Generate the buffer
}

void glUniformBufferObject::ChangeSubData(GLintptr offset,	GLsizeiptr size, const GLvoid *data){
	GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, _UBOid));
	GLCheck(glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data));
    GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

bool glUniformBufferObject::bindUniform(GLuint shaderProgramHandle, GLuint uboLocation) {
	if(uboLocation == GL_INVALID_INDEX) {
       ERROR_FN(Locale::get("ERROR_UBO_INVALID_LOCATION"),shaderProgramHandle);
        return false;
    }

	GLCheck(glUniformBlockBinding(shaderProgramHandle, uboLocation, _bindIndex));
	return true;
}

bool glUniformBufferObject::bindBufferRange(GLintptr offset, GLsizeiptr size) {
	GLCheck(glBindBufferRange(GL_UNIFORM_BUFFER, _bindIndex, _UBOid, offset, size));
	return true;
}

bool glUniformBufferObject::bindBufferBase() {
	GLCheck(glBindBufferBase(GL_UNIFORM_BUFFER, _bindIndex, _UBOid));
	return true;
}