#include "Headers/glLightUniformBufferObject.h"
#include "Rendering/Lighting/Headers/Light.h"

glLightUniformBufferObject::glLightUniformBufferObject() : glUniformBufferObject()
{
}

glLightUniformBufferObject::~glLightUniformBufferObject()
{
}

void glLightUniformBufferObject::ReserveBuffer(GLuint primitiveCount){
	assert(_UBOid != 0);
	GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, _UBOid));
	GLCheck(glBufferData(GL_UNIFORM_BUFFER, sizeof(LightProperties) * primitiveCount, NULL, _usage));
	GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}