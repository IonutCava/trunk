#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

void computeLightVectors(in mat3 normalMatrixW, in mat3 normalMatrixWV) {
    VAR._normalW = normalize(normalMatrixW * dvd_Normal);
    VAR._normalWV = normalize(normalMatrixWV * dvd_Normal);
    VAR._viewDirectionWV = normalize(-VAR._vertexWV.xyz);

#if defined(COMPUTE_TBN)
    vec3 tangent = normalize(normalMatrixWV * dvd_Tangent);
    VAR._tbn = mat3(
                    tangent,
                    normalize(cross(VAR._normalWV, tangent)),
                    VAR._normalWV
                );
#endif
}

#endif //_LIGHTING_DEFAULTS_VERT_