#include "Headers/glMaterialUniformBufferObject.h"
#include "Geometry/Material/Headers/Material.h"

glMaterialUniformBufferObject::glMaterialUniformBufferObject() : glUniformBufferObject(),
																 _mat(NULL)
{
}

glMaterialUniformBufferObject::~glMaterialUniformBufferObject()
{
}

void glMaterialUniformBufferObject::ReserveBuffer(GLuint primitiveCount) {
	assert(_UBOid != 0);
    GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, _UBOid));
    GLCheck(glBufferData(GL_UNIFORM_BUFFER, sizeof(_mat->getShaderData()) * primitiveCount, NULL, _usage));
    GLCheck(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}