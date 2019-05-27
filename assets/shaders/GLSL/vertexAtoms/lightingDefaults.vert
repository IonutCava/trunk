#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

#include "lightInput.cmn"

void computeLightVectors() {
    mat3 normalMatrixWV = dvd_NormalMatrixWV(VAR.dvd_baseInstance);

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._normalWV = normalize(normalMatrixWV * dvd_Normal);
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