-- Compute

#define dvd_dataFlag 0

#include "HiZCullingAlgorithm.cmn";
#include "vegetationData.cmn"
#include "waterData.cmn"

uniform float dvd_treeVisibilityDistance;
uniform float dvd_grassVisibilityDistance;
uniform uint dvd_terrainChunkOffset;

layout(local_size_x = WORK_GROUP_SIZE) in;

float saturate(float v) { return clamp(v, 0.0, 1.0); }

vec3 rotate_vertex_position(vec3 position, vec4 q) {
    vec3 v = position.xyz;
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

uniform vec4 treeExtents;
uniform vec4 grassExtents;
#if defined(CULL_TREES)
#   define MAX_INSTANCES MAX_TREE_INSTANCES
#   define Data treeData
#   define Extents treeExtents
#   define dvd_visibilityDistance dvd_treeVisibilityDistance
#else //CULL_TREES
#   define MAX_INSTANCES MAX_GRASS_INSTANCES
#   define Data grassData
#   define Extents grassExtents
#   define dvd_visibilityDistance dvd_grassVisibilityDistance
#endif //CULL_TREES

void main(void) {

    if (gl_GlobalInvocationID.x >= MAX_INSTANCES) {
        return;
    }

    uint idx = dvd_terrainChunkOffset * MAX_INSTANCES + gl_GlobalInvocationID.x;

    VegetationData instance = Data[idx];
    Data[idx].data.z = 1.0f;

    vec4 positionW = vec4(instance.positionAndScale.xyz, 1.0f);

    float dist = distance(positionW.xyz, dvd_cameraPosition.xyz);

    // Too far away // ToDo: underwater check:
    if (dist > dvd_visibilityDistance || IsUnderWater(positionW.xyz)) {
        Data[idx].data.z = 3.0f;
        return;
    }

    vec3 extents = Extents.xyz;
    float scale = instance.positionAndScale.w;

    Data[idx].data.w = extents.y * scale;

    if (zBufferCullRasterGrid(positionW.xyz, (extents * scale) * 1.1f)) {
        Data[idx].data.z = 3.0f;
    } else {
#       if defined(CULL_TREES)
            Data[idx].data.z = dist > (dvd_visibilityDistance * 0.33f) ? 2.0f : 1.0f;
#       else //CULL_TREES
            const float minDist = 0.01f;
            Data[idx].data.z = saturate((dist - minDist) / (dvd_visibilityDistance - minDist));
#       endif //CULL_TREES
    }
}