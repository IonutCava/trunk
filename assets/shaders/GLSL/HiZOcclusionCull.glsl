-- Compute
#include "HiZCullingAlgorithm.cmn";

layout(binding = BUFFER_ATOMIC_COUNTER, offset = 0) uniform atomic_uint culledCount;

//ref: http://malideveloper.arm.com/resources/sample-code/occlusion-culling-hierarchical-z/
struct NodeData {
    mat4 _worldMatrix;
    mat4 _normalMatrix;
    mat4 _colourMatrix;
    //Temp. w - unused
    vec4 _bbHalfExtents;
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
    IndirectDrawCommand dvd_drawCommands[MAX_VISIBLE_NODES];
};

#define dvd_dataFlag(X) int(dvd_Matrices[X]._colourMatrix[3].w)

layout(location = 0) uniform uint dvd_numEntities = 0;

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
    // Skip occlusion cull if the flag is negative
    if (dvd_dataFlag(nodeIndex) < 0) {
        return;
    }

    const vec4 bSphere = dvd_Matrices[nodeIndex]._normalMatrix[3];
    const vec3 center = bSphere.xyz;
    const float radius = bSphere.w;

    vec3 extents = vec3(radius);//dvd_Matrices[nodeIndex]._bbHalfExtents.xyz;
    if (HiZCull(center, extents, radius)) {
        atomicCounterIncrement(culledCount);
        //dvd_drawCommands[ident].instanceCount = 0;
    }
}

