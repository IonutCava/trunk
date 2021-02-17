#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

#if defined(DEPTH_PASS)

#define computeLightVectors(data)

#else //DEPTH_PASS

#if defined(COMPUTE_TBN) || defined(NEED_TANGENT)
mat3 computeTBN(in mat3 normalMatrixW) {
    const vec3 N = normalize(normalMatrixW * dvd_Normal);
    vec3 T = normalize(normalMatrixW * dvd_Tangent);
    // re-orthogonalize T with respect to N (Gram-Schmidt)
    T = normalize(T - dot(T, N) * N);
    const vec3 B = cross(N, T);
    return mat3(dvd_ViewMatrix) * mat3(T, B, N);
}
#endif //COMPUTE_TBN || NEED_TANGNET

void computeViewDirectionWV(in NodeTransformData data) {
    const vec3 cameraDirection = normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);
    VAR._viewDirectionWV = normalize(mat3(dvd_ViewMatrix) * cameraDirection);
}

void computeLightVectors(in NodeTransformData data) {
    computeViewDirectionWV(data);

    const mat3 normalMatrixW = dvd_NormalMatrixW(data);
#if defined(COMPUTE_TBN)
    VAR._tbnWV = computeTBN(normalMatrixW);
    VAR._normalWV = VAR._tbnWV[2];
#else // COMPUTE_TBN
    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * 
                              normalMatrixW *
                              dvd_Normal);
#endif // COMPUTE_TBN
}

#endif  // DEPTH_PASS

#endif //_LIGHTING_DEFAULTS_VERT_
