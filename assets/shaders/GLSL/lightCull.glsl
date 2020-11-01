--Compute

//Most of the stuff is from here:https://github.com/Angelo1211/HybridRenderingEngine/blob/master/assets/shaders/ComputeShaders/clusterCullLightShader.comp
//With a lot of : https://github.com/pezcode/Cluster

#include "lightInput.cmn"

#define DIRECTIONAL_LIGHT_COUNT dvd_LightData.x
#define POINT_LIGHT_COUNT       dvd_LightData.y
#define SPOT_LIGHT_COUNT        dvd_LightData.z
#define GROUP_SIZE (GRID_SIZE_X_THREADS * GRID_SIZE_Y_THREADS * GRID_SIZE_Z_THREADS)

struct TempLight
{
    vec3 position;
    float radius;
    uint type;
};

shared TempLight sharedLights[GROUP_SIZE];

layout(local_size_x = GRID_SIZE_X_THREADS, local_size_y = GRID_SIZE_Y_THREADS, local_size_z = GRID_SIZE_Z_THREADS) in;

bool lightIntersectsCluster(TempLight light, VolumeTileAABB cluster);

void main(void)
{
    uint visiblePointLights[MAX_LIGHTS_PER_CLUSTER];
    uint visibleSpotLights[MAX_LIGHTS_PER_CLUSTER];
    uint visiblePointCount = 0;
    uint visibleSpotCount = 0;

    // the way we calculate the index doesn't really matter here since we write to the same index in the light grid as we read from the cluster buffer
    uint clusterIndex = gl_GlobalInvocationID.z * gl_WorkGroupSize.x * gl_WorkGroupSize.y +
                        gl_GlobalInvocationID.y * gl_WorkGroupSize.x +
                        gl_GlobalInvocationID.x;

    VolumeTileAABB cluster = cluster[clusterIndex];

    // reset the atomic counter
    // writable compute buffers can't be updated by CPU so do it here
    if (clusterIndex == 0) {
        globalIndexCount = 0;
    }

    const uint lightCount = POINT_LIGHT_COUNT + SPOT_LIGHT_COUNT;
    uint lightOffset = 0;
    while (lightOffset < lightCount)
    {
        // read GROUP_SIZE lights into shared memory
        // each thread copies one light
        uint batchSize = min(GROUP_SIZE, lightCount - lightOffset);

        if (uint(gl_LocalInvocationIndex) < batchSize) {
            uint lightIndex = lightOffset + gl_LocalInvocationIndex;
            Light source = dvd_LightSource[lightIndex + DIRECTIONAL_LIGHT_COUNT];
            if (source._TYPE == LIGHT_SPOT) {
                vec3 position = source._positionWV.xyz;
                float range = source._SPOT_CONE_SLANT_HEIGHT * 0.5f;//range to radius conversion
                sharedLights[gl_LocalInvocationIndex].position = position + source._directionWV.xyz * range;
                sharedLights[gl_LocalInvocationIndex].radius   = range;
                sharedLights[gl_LocalInvocationIndex].type = 1;
            } else {
                sharedLights[gl_LocalInvocationIndex].position = source._positionWV.xyz;
                sharedLights[gl_LocalInvocationIndex].radius   = source._positionWV.w;
                sharedLights[gl_LocalInvocationIndex].type = 0;
            }
        }

        // wait for all threads to finish copying
        barrier();

        // each thread is one cluster and checks against all lights in the cache
        for (uint i = 0; i < batchSize; ++i) {
            if ((visiblePointCount + visibleSpotCount) < MAX_LIGHTS_PER_CLUSTER &&
                lightIntersectsCluster(sharedLights[i], cluster))
            {
                if (sharedLights[i].type == 0) {
                    visiblePointLights[visiblePointCount] = lightOffset + i;
                    visiblePointCount += 1;
                } else {
                    visibleSpotLights[visibleSpotCount] = lightOffset + i;
                    visibleSpotCount += 1;
                }
            }
        }

        lightOffset += batchSize;
    }

    // wait for all threads to finish checking lights
    barrier();

    // get a unique index into the light index list where we can write this cluster's lights
    uint offset = atomicAdd(globalIndexCount, visiblePointCount + visibleSpotCount);

    // copy indices of lights
    for (uint i = 0; i < visiblePointCount; ++i) {
        globalLightIndexList[offset + i] = visiblePointLights[i];
    }
    for (uint i = 0; i < visibleSpotCount; ++i) {
        globalLightIndexList[visiblePointCount + offset + i] = visibleSpotLights[i];
    }

    lightGrid[clusterIndex].offset = offset;
    lightGrid[clusterIndex].countPoint = visiblePointCount;
    lightGrid[clusterIndex].countSpot = visibleSpotCount;
}

// check if light radius extends into the cluster
bool lightIntersectsCluster(TempLight light, VolumeTileAABB cluster) {
    // NOTE: expects light.position to be in view space like the cluster bounds
    // global light list has world space coordinates, but we transform the
    // coordinates in the shared array of lights after copying

    // only add distance in either dimension if it's outside the bounding box
    vec3 belowDist = cluster.minPoint.xyz - light.position;
    vec3 aboveDist = light.position - cluster.maxPoint.xyz;

    vec3 isBelow = vec3(greaterThan(belowDist, vec3(0.0)));
    vec3 isAbove = vec3(greaterThan(aboveDist, vec3(0.0)));

    vec3 distSqVec = (isBelow * belowDist) + (isAbove * aboveDist);
    float distsq = dot(distSqVec, distSqVec);

    return distsq <= (light.radius * light.radius);
}

