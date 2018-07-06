#ifndef _LIGHTING_DEFAULTS_VERT_
#define _LIGHTING_DEFAULTS_VERT_

#include "lightInput.cmn"

void computeLightVectors(){
    if (dvd_lodLevel > 2) {
        for (int i = 0; i < MAX_LIGHT_TYPES; ++i) {
            VAR._lightCount[i] = min(1, dvd_lightCountPerType[i]);
        }
    } else {
        for (int i = 0; i < MAX_LIGHT_TYPES; ++i) {
            VAR._lightCount[i] =  dvd_lightCountPerType[i];
        }
    }
    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;

    VAR._normalWV = normalize(dvd_NormalMatrixWV() * dvd_Normal);
#if defined(COMPUTE_TBN)
    VAR._tangentWV = normalize(dvd_NormalMatrixWV() * dvd_Tangent);
    VAR._bitangentWV = normalize(cross(VAR._normalWV, VAR._tangentWV));
#endif
}

#endif //_LIGHTING_DEFAULTS_VERT_