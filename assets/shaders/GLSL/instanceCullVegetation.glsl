-- Compute

#define dvd_dataFlag 0

#include "HiZCullingAlgorithm.cmn";
#include "vegetationData.cmn"
#include "waterData.cmn"

uniform float dvd_treeVisibilityDistance;
uniform float dvd_grassVisibilityDistance;
uniform uint dvd_terrainChunkOffset;
uniform vec3 cameraPosition;

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

    vec4 positionW = vec4(instance.positionAndScale.xyz, 1.0f);

    const float dist = distance(positionW.xz, cameraPosition.xz);

    // Too far away // ToDo: underwater check:
    if (dist > dvd_visibilityDistance || IsUnderWater(positionW.xyz)) {
        Data[idx].data.z = 3.0f;
        return;
    }

    const float scale = instance.positionAndScale.w;
    const vec3 extents = Extents.xyz * scale * 1.2f;
    const float radius = max(max(extents.x, extents.y), extents.z);

    Data[idx].data.w = extents.y;

    if (HiZCull(positionW.xyz, extents, radius)) {
        Data[idx].data.z = 3.0f;
    } else {
#       if defined(CULL_TREES)
            Data[idx].data.z = dist > (dvd_visibilityDistance * 0.33f) ? 2.0f : 1.0f;
#       else //CULL_TREES
            const float minDistance = dvd_visibilityDistance * 0.33f;
            Data[idx].data.z = max(1.0f - smoothstep(dvd_visibilityDistance * 0.25f, dvd_visibilityDistance * 0.99f, dist), 0.05f);
#       endif //CULL_TREES
    }
}