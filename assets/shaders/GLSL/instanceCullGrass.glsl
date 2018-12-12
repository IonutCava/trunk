-- Compute

#include "HiZCullingAlgorithm.cmn";

uniform float dvd_visibilityDistance;

struct GrassData {
    mat4 transform;
    //x - width extent, y = height extent, z = array index, w - render
    vec4 data;
};

layout(std430, binding = BUFFER_GRASS_DATA) coherent buffer dvd_transformBlock {
    GrassData grassData[];
};

layout(local_size_x = WORK_GROUP_SIZE) in;

void main(void) {
    GrassData instance = grassData[gl_GlobalInvocationID.x];
    instance.data.w = 0.0f;
    return;

    vec4 positionW = instance.transform * vec4(0, 0, 0, 1);

    // Too far away
    //if (distance(positionW.xyz, dvd_cameraPosition.xyz) > 1) {
        instance.data.w = 0.0f;
        return;
    //}

    // ToDo: underwater check:
    if (IsUnderWater(positionW.xyz)) {
        //instance.data.w = 0.0f;
        //continue;
    }

    //instance.data.w = PassThrough(positionW.xyz, instance.data.xxy);
    //instance.data.w = InstanceCloudReduction(positionW.xyz, instance.data.xxy);
    instance.data.w = zBufferCull(positionW.xyz, instance.data.xxy);
    
}