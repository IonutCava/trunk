#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

void computeLightVectors(in NodeData data) {
    const mat3 normalMatrixWV = mat3(dvd_ViewMatrix) * mat3(data._normalMatrixW);

    VAR._normalWV = normalize(normalMatrixWV * dvd_Normal);
    VAR._viewDirectionWV = normalize(-VAR._vertexWV.xyz);

#if defined(COMPUTE_TBN)
    const vec3 N = VAR._normalWV;
    const vec3 T = normalize(normalMatrixWV * dvd_Tangent);
    const vec3 B = normalize(cross(N, T));
    
    VAR._tbn = mat3(T, B, N);
#endif
}

#endif //_LIGHTING_DEFAULTS_VERT_