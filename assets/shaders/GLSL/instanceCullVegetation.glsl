-- Compute

#define dvd_dataFlag 0
#define INVS_SQRT_3 0.57735026919f
#if defined(CULL_TREES)
uniform float dvd_treeVisibilityDistance;
uniform vec4 treeExtents;
#else
uniform float dvd_grassVisibilityDistance;
uniform vec4 grassExtents;
#endif

uniform uint  dvd_terrainChunkOffset;

#define NEED_SCENE_DATA
#include "HiZCullingAlgorithm.cmn";
#include "vegetationData.cmn"
#include "waterData.cmn"

vec3 rotate_vertex_position(vec3 position, vec4 q) {
    vec3 v = position.xyz;
    return v + 2.0f * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

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

void CullItem(in uint idx) {
    Data[idx].data.z = 3.0f;
}

layout(local_size_x = WORK_GROUP_SIZE) in;
void main(void) {

    const uint ident = gl_GlobalInvocationID.x;
    const uint nodeIndex = (dvd_terrainChunkOffset * MAX_INSTANCES) + ident;
    if (ident >= MAX_INSTANCES) {
        CullItem(nodeIndex);
        return;
    }

    VegetationData instance = Data[nodeIndex];

    const float scale    = instance.positionAndScale.w;
    const vec4 extents   = Extents * scale;

    vec3 vert = vec3(0.0f, extents.y * 0.5f, 0.0f);
    vert = rotate_vertex_position(vert * scale, instance.orientationQuad);
    const vec3 positionW = vert + instance.positionAndScale.xyz;

    const float dist = distance(positionW.xz, dvd_cameraPosition.xz);
    Data[nodeIndex].data.z = 1.0f;
    // Too far away
    if (dist > dvd_visibilityDistance) {
        CullItem(nodeIndex);
        return;
    }

    if (HiZCull(positionW, extents.xyz, extents.w) || IsUnderWater(positionW.xyz)) {
        CullItem(nodeIndex);
    } else {
#       if defined(CULL_TREES)
            Data[nodeIndex].data.z = dist > (dvd_visibilityDistance * 0.33f) ? 2.0f : 1.0f;
#       else //CULL_TREES
            Data[nodeIndex].data.z = max(1.0f - smoothstep(dvd_visibilityDistance * 0.85f, dvd_visibilityDistance * 0.995f, dist), 0.05f);
#       endif //CULL_TREES
        Data[nodeIndex].data.w = dist;
    }
}