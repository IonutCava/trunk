-- Compute

#include "HiZCullingAlgorithm.cmn";

uniform float dvd_visibilityDistance;

struct GrassData {
    mat4 transform;
    vec4 positionAndIndex;
    vec4 extentAndRender;
};

layout(std430, binding = BUFFER_GRASS_DATA) coherent buffer dvd_transformBlock {
    GrassData grassData[];
};

uniform uint instanceCount = 1;
uniform uint cullType = 0;

layout(local_size_x = 64) in;

int occlusionCull(in GrassData data) {
    
    vec4 positionW = data.transform * vec4(0, 0, 0, 1);

    if (distance(dvd_ViewMatrix * positionW, vec4(0.0, 0.0, 0.0, 1.0)) > dvd_visibilityDistance) {
        return 0;
    }

    switch (cullType) {
        case 0 :
            return PassThrough(positionW.xyz, data.extentAndRender.xyz);
        case 1 : 
            return InstanceCloudReduction(positionW.xyz, data.extentAndRender.xyz);
    };

    return zBufferCull(positionW.xyz, data.extentAndRender.xyz);
                           
}

void main(void) {
    for (uint i = 0; i < instanceCount; ++i) {
        GrassData data = grassData[i];
        data.extentAndRender.w = occlusionCull(data);
    }
}