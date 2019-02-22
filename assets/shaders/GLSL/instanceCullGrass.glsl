-- Compute

#include "HiZCullingAlgorithm.cmn";

uniform float dvd_visibilityDistance;

struct GrassData {
    vec4 positionAndScale;
    vec4 orientationQuad;
    //x - width extent, y = height extent, z = array index, w - lod
    vec4 data;
};

layout(std430, binding = BUFFER_GRASS_DATA) coherent buffer dvd_transformBlock {
    GrassData grassData[MAX_INSTANCES];
};

layout(local_size_x = WORK_GROUP_SIZE) in;

float saturate(float v) { return clamp(v, 0.0, 1.0); }

vec3 rotate_vertex_position(vec3 position, vec4 q) {
    vec3 v = position.xyz;
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main(void) {
    if (gl_GlobalInvocationID.x >= MAX_INSTANCES) {
        return;
    }

    float minDist = 0.01f;
    GrassData instance = grassData[gl_GlobalInvocationID.x];

    vec4 positionW = vec4(instance.positionAndScale.xyz, 1.0f);

    // Too far away
    float dist = distance(positionW.xyz, dvd_cameraPosition.xyz);
    if (dist > dvd_visibilityDistance) {
        grassData[gl_GlobalInvocationID.x].data.w = 3.0f;
        return;
    }
    // ToDo: underwater check:
    if (IsUnderWater(positionW.xyz)) {
        grassData[gl_GlobalInvocationID.x].data.w = 3.0f;
        return;
    }

    //grassData[gl_GlobalInvocationID.x].data.w = PassThrough(positionW.xyz, instance.data.xxy * instance.positionAndScale.w);
    //grassData[gl_GlobalInvocationID.x].data.w = InstanceCloudReduction(positionW.xyz, instance.data.xxy * instance.positionAndScale.w);

    if (zBufferCull(positionW.xyz, (instance.data.xxy * 1.05f) * instance.positionAndScale.w) > 0) {
        grassData[gl_GlobalInvocationID.x].data.w = saturate((dist - minDist) / (dvd_visibilityDistance - minDist));
    } else {
        grassData[gl_GlobalInvocationID.x].data.w = 3.0f;
    }
}