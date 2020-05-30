#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

void computeLightVectors(in NodeData data) {
    const mat3 normalMatrixWV = mat3(dvd_ViewMatrix) * mat3(data._normalMatrixW);

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