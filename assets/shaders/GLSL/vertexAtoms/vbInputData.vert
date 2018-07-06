#ifndef _VB_INPUT_DATA_VERT_
#define _VB_INPUT_DATA_VERT_

#include "nodeBufferedInput.cmn"

#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif

vec4   dvd_Vertex;
vec4   dvd_Colour;
vec3   dvd_Normal;
vec3   dvd_Tangent;

vec3 UNPACK_FLOAT(in float value) {
    return (fract(vec3(1.0, 256.0, 65536.0) * value)* 2.0) - 1.0;
}

void computeData(){
    VAR.dvd_drawID  = gl_BaseInstanceARB;
    dvd_Vertex  = vec4(inVertexData, 1.0);
    //Occlusion culling visibility debug code
#if defined(USE_HIZ_CULLING) && defined(DEBUG_HIZ_CULLING)
    if (dvd_customData > 2.0) {
        dvd_Vertex.xyz *= 5;
    }
#endif
    dvd_Normal  = UNPACK_FLOAT(inNormalData);
    dvd_Colour   = inColourData;
    dvd_Tangent = UNPACK_FLOAT(inTangentData); 

#   if defined(USE_GPU_SKINNING)
#       if defined(COMPUTE_TBN)
            applyBoneTransforms(dvd_Vertex, dvd_Normal, dvd_Tangent, dvd_lodLevel);
#       else
            applyBoneTransforms(dvd_Vertex, dvd_Normal, dvd_lodLevel);
#       endif
#   endif

    VAR._texCoord = inTexCoordData;
    VAR._vertexW  = dvd_WorldMatrix(VAR.dvd_drawID) * dvd_Vertex;

    setClipPlanes(VAR._vertexW);
}

#endif //_VB_INPUT_DATA_VERT_