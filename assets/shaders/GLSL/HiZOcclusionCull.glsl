--Compute
#include "HiZCullingAlgorithm.cmn";
#include "nodeDataDefinition.cmn"

#define INVS_SQRT_3 0.57735026919f

ADD_UNIFORM(uint, dvd_numEntities)
ADD_UNIFORM(uint, dvd_countCulledItems)

layout(binding = BUFFER_ATOMIC_COUNTER, offset = 0) uniform atomic_uint culledCount;

//ref: http://malideveloper.arm.com/resources/sample-code/occlusion-culling-hierarchical-z/

struct IndirectDrawCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

layout(binding = BUFFER_NODE_TRANSFORM_DATA, std430) coherent COMP_ONLY_R buffer dvd_TransformBlock
{
    NodeTransformData dvd_Transforms[MAX_VISIBLE_NODES];
};


layout(binding = BUFFER_GPU_COMMANDS, std430) coherent COMP_ONLY_RW buffer dvd_GPUCmds
{
    IndirectDrawCommand dvd_drawCommands[];
};

void CullItem(in uint idx) {
    if (dvd_countCulledItems == 1u) {
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

    const uint transformIndex = dvd_drawCommands[ident].baseInstance >> 16;
    // We dont currently handle instanced nodes with this. We might need to in the future
    // Usually this is just terrain, vegetation and the skybox. So not that bad all in all since those have
    // their own culling routines
    if (transformIndex == 0u) {
        return;
    }

    // Skip occlusion cull if the flag is set
    if (!dvd_cullNode(dvd_Transforms[transformIndex])) {
        return;
    }

    const vec3 boundsCenter = dvd_Transforms[transformIndex]._normalMatrixW[3].xyz;
    const vec2 bboxHalfExtentsXY = unpackHalf2x16(uint(dvd_Transforms[transformIndex]._normalMatrixW[1][3]));
    const vec2 bboxHalfExtentsZRadius = unpackHalf2x16(uint(dvd_Transforms[transformIndex]._normalMatrixW[2][3]));

    const vec3 bBoxHExtents = vec3(bboxHalfExtentsXY, bboxHalfExtentsZRadius.x);

    if (HiZCull(boundsCenter, bBoxHExtents, bboxHalfExtentsZRadius.y)) {
        CullItem(ident);
    }
}

