#ifndef _VB_INPUT_DATA_VERT_
#define _VB_INPUT_DATA_VERT_

invariant gl_Position;

#include "nodeBufferedInput.cmn"

#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif

vec4   dvd_Vertex;
vec4   dvd_Color;
vec3   dvd_Normal;
vec3   dvd_Tangent;

vec3 UNPACK_FLOAT(in float value) {
    return (fract(vec3(1.0, 256.0, 65536.0) * value)* 2.0) - 1.0;
}

void computeData(){
    VAR.dvd_drawID  = gl_BaseInstanceARB;// + gl_DrawIDARB;
    dvd_Vertex  = vec4(inVertexData,1.0);
    dvd_Normal  = UNPACK_FLOAT(inNormalData);
    dvd_Color   = inColorData;
    dvd_Tangent = UNPACK_FLOAT(inTangentData); 

#   if defined(USE_GPU_SKINNING)
#       if defined(COMPUTE_TBN)
            applyBoneTransforms(dvd_Vertex, dvd_Normal, dvd_Tangent, dvd_lodLevel);
#       else
            applyBoneTransforms(dvd_Vertex, dvd_Normal, dvd_lodLevel);
#       endif
#   endif

    VAR._texCoord = inTexCoordData;
    VAR._vertexW  = dvd_WorldMatrix() * dvd_Vertex;

    setClipPlanes(VAR._vertexW);
}

#endif //_VB_INPUT_DATA_VERT_