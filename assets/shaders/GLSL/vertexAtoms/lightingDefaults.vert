#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

#include "lightInput.cmn"

out vec4 _vertexWV;

void computeLightVectors(){
    _lightCount = dvd_lightCount;
    _normalWV = normalize(dvd_NormalMatrix * dvd_Normal); //<ModelView Normal 
    _vertexWV = dvd_ViewMatrix * _vertexW;

#if defined(COMPUTE_TBN)
    _tangentWV = normalize(dvd_NormalMatrix * dvd_Tangent);
    _bitangentWV = normalize(cross(_normalWV, _tangentWV));
    
    vec3 tmpVec = -_vertexWV.xyz;
   _viewDirection.x = dot(tmpVec, _tangentWV);
   _viewDirection.y = dot(tmpVec, _bitangentWV);
   _viewDirection.z = dot(tmpVec, _normalWV);
#else
    _viewDirection = -_vertexWV.xyz;
#endif
    _viewDirection = normalize(_viewDirection);
}

#endif //_LIGHTING_DEFAULTS_VERT_