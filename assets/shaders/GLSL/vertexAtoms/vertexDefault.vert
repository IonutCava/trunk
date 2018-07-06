#ifndef _VERTEX_DEFAULT_VERT_
#define _VERTEX_DEFAULT_VERT_

void computeData()
{
    VAR._texCoord = inTexCoordData;
    gl_Position = dvd_ViewProjectionMatrix * dvd_WorldMatrix(VAR.dvd_drawID) * vec4(inVertexData,1.0);
}

#endif //_VERTEX_DEFAULT_VERT_