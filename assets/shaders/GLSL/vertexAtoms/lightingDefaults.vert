#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

#include "lightInput.cmn"

void computeLightVectors() {
    mat3 normalMatrixWV = dvd_NormalMatrixWV(VAR.dvd_instanceID);

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._normalWV = normalize(normalMatrixWV * dvd_Normal);
#if defined(COMPUTE_TBN)
    VAR._tangentWV = normalize(normalMatrixWV * dvd_Tangent);
    VAR._bitangentWV = normalize(cross(VAR._normalWV, VAR._tangentWV));
#endif
}

#endif //_LIGHTING_DEFAULTS_VERT_