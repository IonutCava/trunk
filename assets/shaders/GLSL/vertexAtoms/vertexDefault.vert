#ifndef _VERTEX_DEFAULT_VERT_
#define _VERTEX_DEFAULT_VERT_

out vec2 _texCoord;

void computeData()
{
    _texCoord = inTexCoordData;
    gl_Position = dvd_ViewProjectionMatrix * dvd_WorldMatrix() * vec4(inVertexData,1.0);
}

#endif //_VERTEX_DEFAULT_VERT_