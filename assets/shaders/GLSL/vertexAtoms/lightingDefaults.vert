#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

#include "lightInput.cmn"

void computeLightVectors(){
    VAR._lightCount = dvd_lodLevel > 2 ? 1 : dvd_lightCount;
    VAR._normalWV = normalize(dvd_NormalMatrix() * dvd_Normal);
    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;

#if defined(COMPUTE_TBN)
    VAR._tangentWV = normalize(dvd_NormalMatrix() * dvd_Tangent);
    VAR._bitangentWV = normalize(cross(VAR._normalWV, VAR._tangentWV));
    
    vec3 tmpVec = -VAR._vertexWV.xyz;
    VAR._viewDirection.x = dot(tmpVec, VAR._tangentWV);
    VAR._viewDirection.y = dot(tmpVec, VAR._bitangentWV);
    VAR._viewDirection.z = dot(tmpVec, VAR._normalWV);
#else
    VAR._viewDirection = -VAR._vertexWV.xyz;
#endif
    VAR._viewDirection = normalize(VAR._viewDirection);
}

#endif //_LIGHTING_DEFAULTS_VERT_