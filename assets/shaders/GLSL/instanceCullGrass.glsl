-- Compute

#include "HiZCullingAlgorithm.cmn";

uniform float dvd_visibilityDistance;

struct GrassData {
    mat4 transform;
    //x - width extent, y = height extent, z = array index, w - lod
    vec4 data;
};

layout(std430, binding = BUFFER_GRASS_DATA) coherent buffer dvd_transformBlock {
    GrassData grassData[];
};

layout(local_size_x = WORK_GROUP_SIZE) in;

float saturate(float v) { return clamp(v, 0.0, 1.0); }

void main(void) {
    float minDist = 0.01f;
    GrassData instance = grassData[gl_GlobalInvocationID.x];
    vec4 positionW = instance.transform * vec4(0, 0, 0, 1);

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

    grassData[gl_GlobalInvocationID.x].data.w = PassThrough(positionW.xyz, instance.data.xxy);
    grassData[gl_GlobalInvocationID.x].data.w = InstanceCloudReduction(positionW.xyz, instance.data.xxy);
    if (zBufferCull(positionW.xyz, instance.data.xxy) > 0) {
        grassData[gl_GlobalInvocationID.x].data.w = saturate((dist - minDist) / (dvd_visibilityDistance - minDist));
    } else {
        grassData[gl_GlobalInvocationID.x].data.w = 3.0f;
    }
}