-- Compute

#define dvd_occlusionCullFlag 0

#include "HiZCullingAlgorithm.cmn";
#include "vegetationData.cmn"

uniform float dvd_visibilityDistance;
uniform uint offset;

layout(local_size_x = WORK_GROUP_SIZE) in;

float saturate(float v) { return clamp(v, 0.0, 1.0); }

vec3 rotate_vertex_position(vec3 position, vec4 q) {
    vec3 v = position.xyz;
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

#if defined(CULL_TREES)
#   define MAX_INSTANCES MAX_TREE_INSTANCES
#   define VegData treeData
#else //CULL_TREES
#   define MAX_INSTANCES MAX_GRASS_INSTANCES
#   define VegData grassData
#endif //CULL_TREES

#define Data(X) VegData[offset * MAX_INSTANCES + X]

void main(void) {

    if (gl_GlobalInvocationID.x >= MAX_INSTANCES) {
        return;
    }

    float minDist = 0.01f;
    VegetationData instance = Data(gl_GlobalInvocationID.x);

    vec4 positionW = vec4(instance.positionAndScale.xyz, 1.0f);

#if !defined(CULL_TREES)
    // Too far away
    float dist = distance(positionW.xyz, dvd_cameraPosition.xyz);
    if (dist > dvd_visibilityDistance) {
        Data(gl_GlobalInvocationID.x).data.w = 3.0f;
        return;
    }
    // ToDo: underwater check:
    if (IsUnderWater(positionW.xyz)) {
        Data(gl_GlobalInvocationID.x).data.w = 3.0f;
        return;
    }
    //Data(gl_GlobalInvocationID.x).data.w = PassThrough(positionW.xyz, instance.data.xxy * instance.positionAndScale.w);
    //Data(gl_GlobalInvocationID.x).data.w = InstanceCloudReduction(positionW.xyz, instance.data.xxy * instance.positionAndScale.w);

#endif //CULL_TREES

    vec3 dim = UNPACK_FLOAT(instance.data.x);
    if (zBufferCull(positionW.xyz, (dim.xxy * 1.001f) * instance.positionAndScale.w) > 0) {
#       if defined(CULL_TREES)
            Data(gl_GlobalInvocationID.x).data.w = 1.0f;
#       else //CULL_TREES
            Data(gl_GlobalInvocationID.x).data.w = saturate((dist - minDist) / (dvd_visibilityDistance - minDist));
#       endif //CULL_TREES
    } else {
        Data(gl_GlobalInvocationID.x).data.w = 3.0f;
    }
}