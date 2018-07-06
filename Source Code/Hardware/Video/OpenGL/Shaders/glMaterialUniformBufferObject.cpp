#include "Headers/glMaterialUniformBufferObject.h" 
#include "Geometry/Material/Headers/Material.h"

void glMaterialUniformBufferObject::FillData() {
    glBindBuffer(GL_UNIFORM_BUFFER, _UBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(_mat->getShaderData()), (void *)(&_mat->getShaderData()), _usage);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}