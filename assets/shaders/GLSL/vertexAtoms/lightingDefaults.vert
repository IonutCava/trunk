#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

#if (!defined(PRE_PASS) || !defined(USE_DEFFERED_NORMALS)) && !defined(SHADOW_PASS)
#define COMPUTE_NORMALS
#endif //!PRE_PASS || !USE_DEFFERED_NORMALS

void computeLightVectors(in NodeData data) {
#if !defined(USE_MIN_SHADING)

#if (!defined(PRE_PASS) && !defined(SHADOW_PASS)) || defined(COMPUTE_TBN)
    vec3 toCamera = normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);
#endif

#if !defined(PRE_PASS) && !defined(SHADOW_PASS)
    VAR._viewDirectionWV = normalize(mat3(dvd_ViewMatrix) * toCamera);
#endif //!PRE_PASS && !SHADOW_PASS

#if defined(COMPUTE_NORMALS)
    const mat3 normalMatrixW = mat3(data._normalMatrixW);
    const vec3 N = normalize(normalMatrixW * dvd_Normal);

#if defined(COMPUTE_TBN) 
    vec3 T = normalize(normalMatrixW * dvd_Tangent);
    // re-orthogonalize T with respect to N (Gram-Schmidt)
    T = normalize(T - dot(T, N) * N);
    const vec3 B = cross(N, T);
    
    const mat3 TBN = mat3(T, B, N);
    VAR._tbnWV = mat3(dvd_ViewMatrix) * TBN;
    VAR._tbnViewDir = normalize(transpose(TBN) * toCamera);
#endif //COMPUTE_TBN

    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * N);

#else //COMPUTE_NORMALS
#if defined(COMPUTE_TBN) 
    VAR._tbnWV = mat3(1.0f);
    VAR._tbnViewDir = vec3(0.0f);
#endif //COMPUTE_TBN

    VAR._normalWV = vec3(0.0f);
#endif //COMPUTE_NORMALS

#endif //USE_MIN_SHADING
}

#endif //_LIGHTING_DEFAULTS_VERT_