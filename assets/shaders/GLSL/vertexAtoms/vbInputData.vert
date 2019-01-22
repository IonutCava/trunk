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

void computeDataMinimal() {
    VAR.dvd_baseInstance = gl_BaseInstanceARB;
    VAR.dvd_instanceID = gl_InstanceID;
    VAR.dvd_drawID = gl_DrawIDARB;
    VAR._texCoord = inTexCoordData;

    dvd_Vertex = vec4(inVertexData, 1.0);
    dvd_Normal = UNPACK_FLOAT(inNormalData);
    dvd_Colour = inColourData;
    dvd_Tangent = UNPACK_FLOAT(inTangentData);

#   if defined(USE_GPU_SKINNING)
#       if defined(COMPUTE_TBN)
    applyBoneTransforms(dvd_Vertex, dvd_Normal, dvd_Tangent, dvd_lodLevel);
#       else
    applyBoneTransforms(dvd_Vertex, dvd_Normal, dvd_lodLevel);
#       endif
#   endif
}

void computeDataNoClip() {
    computeDataMinimal();

    VAR._texCoord = inTexCoordData;
    VAR._vertexW = dvd_WorldMatrix(VAR.dvd_baseInstance) * dvd_Vertex;
}

void computeData() {
    computeDataNoClip();
    setClipPlanes(VAR._vertexW);
}

// https://www.geeks3d.com/20141201/how-to-rotate-a-vertex-by-a-quaternion-in-glsl/
vec4 quat_conj(vec4 q) {
    return vec4(-q.x, -q.y, -q.z, q.w);
}

vec4 quat_mult(vec4 q1, vec4 q2) {
    vec4 qr;
    qr.x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
    qr.y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
    qr.z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);
    qr.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
    return qr;
}

vec3 rotate_vertex_position(vec3 position, vec4 q) {
    vec3 v = position.xyz;
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

mat4 build_transform(vec3 pos, vec3 eulerAngles)
{
    vec3 ang = eulerAngles;

    float cosX = cos(ang.x);
    float sinX = sin(ang.x);
    float cosY = cos(ang.y);
    float sinY = sin(ang.y);
    float cosZ = cos(ang.z);
    float sinZ = sin(ang.z);

    mat4 m;

    float m00 = cosY * cosZ + sinX * sinY * sinZ;
    float m01 = cosY * sinZ - sinX * sinY * cosZ;
    float m02 = cosX * sinY;
    float m03 = 0.0;

    float m04 = -cosX * sinZ;
    float m05 = cosX * cosZ;
    float m06 = sinX;
    float m07 = 0.0;

    float m08 = sinX * cosY * sinZ - sinY * cosZ;
    float m09 = -sinY * sinZ - sinX * cosY * cosZ;
    float m10 = cosX * cosY;
    float m11 = 0.0;

    float m12 = pos.x;
    float m13 = pos.y;
    float m14 = pos.z;
    float m15 = 1.0;

    /*
    //------ Orientation ---------------------------------
    m[0] = vec4(m00, m01, m02, m03); // first column.
    m[1] = vec4(m04, m05, m06, m07); // second column.
    m[2] = vec4(m08, m09, m10, m11); // third column.

    //------ Position ------------------------------------
    m[3] = vec4(m12, m13, m14, m15); // fourth column.
    */

    //------ Orientation ---------------------------------
    m[0][0] = m00; // first entry of the first column.
    m[0][1] = m01; // second entry of the first column.
    m[0][2] = m02;
    m[0][3] = m03;

    m[1][0] = m04; // first entry of the second column.
    m[1][1] = m05; // second entry of the second column.
    m[1][2] = m06;
    m[1][3] = m07;

    m[2][0] = m08; // first entry of the third column.
    m[2][1] = m09; // second entry of the third column.
    m[2][2] = m10;
    m[2][3] = m11;

    //------ Position ------------------------------------
    m[3][0] = m12; // first entry of the fourth column.
    m[3][1] = m13; // second entry of the fourth column.
    m[3][2] = m14;
    m[3][3] = m15;

    return m;
}
#endif //_VB_INPUT_DATA_VERT_