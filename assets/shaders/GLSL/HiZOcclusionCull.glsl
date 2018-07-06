-- Compute

#include "HiZCullingAlgorithm.cmn";

//ref: http://malideveloper.arm.com/resources/sample-code/occlusion-culling-hierarchical-z/
struct NodeData {
    mat4 _worldMatrix;
    mat4 _normalMatrix;
    mat4 _colorMatrix;
    vec4 _properties;
};

layout(binding = BUFFER_NODE_INFO, std430) coherent buffer dvd_MatrixBlock
{
    NodeData dvd_Matrices[MAX_VISIBLE_NODES];
};

// x - isSelected/isHighlighted; y - isShadowMapped; z - lodLevel, w - reserved
#define dvd_customData(X) dvd_Matrices[X]._properties.w 

layout(location = 0) uniform uint dvd_numEntities;
layout(binding = 0, offset = 0) uniform atomic_uint culledCount;
layout(local_size_x = 64) in;

void main()
{
    uint ident = gl_GlobalInvocationID.x;
    
    if (ident >= dvd_numEntities) {
        dvd_drawCommands[ident].instanceCount = 0;
        return;
    }
    
    uint drawCount = dvd_drawCommands[ident].baseInstance;
    uint nodeIndex = dvd_drawCommands[ident].baseInstance;
    vec4 bSphere = dvd_Matrices[nodeIndex]._normalMatrix[3];
    vec3 center = bSphere.xyz;
    float radius = bSphere.w;

    
    // Sphere clips against near plane, just assume visibility.
    if ((dvd_ViewMatrix * vec4(center, 1.0)).z + radius >= -dvd_ZPlanesCombined.z) {
        return;
    }

    if (zBufferCull(center, vec3(radius)) == 0) {
        atomicCounterIncrement(culledCount);
        dvd_drawCommands[ident].instanceCount = 0;
        dvd_customData(nodeIndex) = 3.0;
    }
}

