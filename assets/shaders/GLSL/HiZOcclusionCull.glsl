-- Compute

#include "HiZCullingAlgorithm.cmn";

//ref: http://malideveloper.arm.com/resources/sample-code/occlusion-culling-hierarchical-z/
struct NodeData {
    mat4 _worldMatrix;
    mat4 _normalMatrix;
    mat4 _colorMatrix;
    vec4 dvd_BufferIntegerValues;
    vec4 dvd_BufferMatProperties;
    vec4 dvd_boundingSphere;
    vec4 _padding_reserved;
};

layout(binding = BUFFER_NODE_INFO, std430) buffer dvd_MatrixBlock
{
    NodeData dvd_Matrices[MAX_VISIBLE_NODES + 1];
};

// x - isSelected/isHighlighted; y - isShadowMapped; z - lodLevel, w - reserved
#define dvd_customData(X) dvd_Matrices[X].dvd_BufferIntegerValues.w 

layout(location = 0) uniform uint dvd_numEntities;
layout(binding = 0, offset = 0) uniform atomic_uint culledCount;
layout(local_size_x = 64) in;
void main()
{
    uint ident = gl_GlobalInvocationID.x;
    if (ident >= dvd_numEntities) {
        atomicCounterIncrement(culledCount);
        dvd_drawCommands[ident].instanceCount = 0;
        return;
    }
    
    uint nodeIndex = dvd_drawCommands[ident].baseInstance;

    vec3 center = dvd_Matrices[nodeIndex].dvd_boundingSphere.xyz;
    float radius = dvd_Matrices[nodeIndex].dvd_boundingSphere.w;

    // Sphere clips against near plane, just assume visibility.
    if ((dvd_ViewMatrix * vec4(center, 1.0)).z + radius >= -dvd_ZPlanesCombined.x) {
        dvd_customData(ident) = 1.0;
        return;
    }

    if (HiZOcclusionCull(center, vec3(radius)) == 0) {
        atomicCounterIncrement(culledCount);
        dvd_drawCommands[ident].instanceCount = 0;
        return;
    }
}

