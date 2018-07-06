#include "Headers/glMatrixUniformBufferObject.h"

glMatrixUniformBufferObject::glMatrixUniformBufferObject()
{
}

glMatrixUniformBufferObject::~glMatrixUniformBufferObject()
{
}

void glMatrixUniformBufferObject::ReserveBuffer(GLuint primitiveCount){
	assert(_UBOid != 0);
	GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, _UBOid));
	GLCheck(glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * primitiveCount, NULL, _usage));
	GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}