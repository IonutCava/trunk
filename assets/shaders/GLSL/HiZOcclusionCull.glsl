-- Compute
#include "HiZCullingAlgorithm.cmn";

layout(binding = BUFFER_ATOMIC_COUNTER, offset = 0) uniform atomic_uint culledCount;

//ref: http://malideveloper.arm.com/resources/sample-code/occlusion-culling-hierarchical-z/
struct NodeData {
    mat4 _worldMatrix;
    mat4 _normalMatrix;
    mat4 _colourMatrix;
};

struct IndirectDrawCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

layout(binding = BUFFER_NODE_INFO, std430) coherent buffer dvd_MatrixBlock
{
    NodeData dvd_Matrices[MAX_VISIBLE_NODES];
};

layout(binding = BUFFER_GPU_COMMANDS, std430) coherent buffer dvd_GPUCmds
{
    IndirectDrawCommand dvd_drawCommands[];
};

#define dvd_dataFlag(X) int(dvd_Matrices[X]._colourMatrix[3].w)

layout(location = 0) uniform uint dvd_numEntities = 0;
layout(location = 1) uniform float dvd_nearPlaneDistance = 0.1f;
layout(location = 2) uniform uint dvd_commandOffset = 0;

layout(local_size_x = 64) in;


void cullNode(const in uint idx) {
    atomicCounterIncrement(culledCount);
    dvd_drawCommands[dvd_commandOffset + idx].instanceCount = 0;
}

void main()
{
    uint ident = gl_GlobalInvocationID.x;
    
    if (ident >= dvd_numEntities) {
        cullNode(ident);
        return;
    }
    
    uint nodeIndex = dvd_drawCommands[dvd_commandOffset + ident].baseInstance;
    // Skip occlusion cull if the flag is negative
    if (dvd_dataFlag(nodeIndex) < 0) {
        return;
    }

    vec4 bSphere = dvd_Matrices[nodeIndex]._normalMatrix[3];
    vec3 center = bSphere.xyz;
    float radius = bSphere.w;
    
    // Sphere clips against near plane, just assume visibility.
    if ((viewMatrix * vec4(center, 1.0)).z + radius >= -dvd_nearPlaneDistance) {
        return;
    }

    if (zBufferCull(center, vec3(radius)) == 0) {
        cullNode(ident);
    }
}

