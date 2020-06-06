#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

void computeLightVectors(in NodeData data) {
    VAR._viewDirectionWV = normalize(-VAR._vertexWV.xyz);

    const mat3 normalMatrixW = mat3(data._normalMatrixW);
    const vec3 N = normalize(normalMatrixW * dvd_Normal);

#if defined(COMPUTE_TBN)
    vec3 T = normalize(normalMatrixW * dvd_Tangent);
    // re-orthogonalize T with respect to N (Gram-Schmidt)
    T = normalize(T - dot(T, N) * N);
    const vec3 B = cross(N, T);
    
    const mat3 TBN = mat3(T, B, N);
    VAR._tbnWV = mat3(dvd_ViewMatrix) * TBN;
    VAR._tbnViewDir = normalize(transpose(TBN) * (dvd_cameraPosition.xyz - VAR._vertexW.xyz));
#endif

    
    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * N);
}

#endif //_LIGHTING_DEFAULTS_VERT_