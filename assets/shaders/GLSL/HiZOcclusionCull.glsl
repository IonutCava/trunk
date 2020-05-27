--Compute
#include "HiZCullingAlgorithm.cmn";
#include "nodeDataDefinition.cmn"

#define INVS_SQRT_3 0.57735026919f

uniform uint dvd_numEntities;
uniform uint countCulledItems = 0u;
layout(binding = BUFFER_ATOMIC_COUNTER, offset = 0) uniform atomic_uint culledCount;

//ref: http://malideveloper.arm.com/resources/sample-code/occlusion-culling-hierarchical-z/

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

void CullItem(in uint idx) {
    if (countCulledItems == 1u) {
        atomicCounterIncrement(culledCount);
    }
    dvd_drawCommands[idx].instanceCount = 0u;
}

layout(local_size_x = WORK_GROUP_SIZE) in;
void main()
{
    const uint ident = gl_GlobalInvocationID.x;

    if (ident >= dvd_numEntities) {
        CullItem(ident);
        return;
    }

    const uint nodeIndex = dvd_drawCommands[ident].baseInstance;
    // We dont currently handle instanced nodes with this. We might need to in the future
    // Usually this is just terrain, vegetation and the skybox. So not that bad all in all since those have
    // their own culling routines
    if (nodeIndex == 0u) {
        return;
    }

    // Skip occlusion cull if the flag is negative
    if (dvd_Matrices[nodeIndex]._colourMatrix[3].w < 0.0f) {
        return;
    }

    const vec4 bSphere = dvd_Matrices[nodeIndex]._normalMatrixW[3];
    const vec3 center = bSphere.xyz;
    const float radius = bSphere.w;
    const vec3 extents = vec3(radius * INVS_SQRT_3);//dvd_Matrices[nodeIndex]._bbHalfExtents.xyz;

    if (HiZCull(center, extents, radius)) {
        CullItem(ident);
    }
}

