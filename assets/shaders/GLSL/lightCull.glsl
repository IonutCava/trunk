--Compute

//ToDo: change to this: https://github.com/bcrusco/Forward-Plus-Renderer/blob/master/Forward-Plus/Forward-Plus/source/shaders/light_culling.comp.glsl
#include "lightInput.cmn"

#define DIRECTIONAL_LIGHT_COUNT dvd_LightData.x
#define POINT_LIGHT_COUNT       dvd_LightData.y
#define SPOT_LIGHT_COUNT        dvd_LightData.z

// shared = The value of such variables are shared between all invocations within a work group.
shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visibleLightCount;
shared vec4 frustumPlanes[6];
shared mat4 viewInverse;
// Shared local storage for visible indices, will be written out to the global buffer at the end
shared int visibleLightIndices[MAX_NUM_LIGHTS_PER_TILE];

//layout(r32f, binding = TEXTURE_DEPTH_MAP) uniform image2D depthBuffer;
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D depthBuffer;

layout(local_size_x = FORWARD_PLUS_TILE_RES, local_size_y = FORWARD_PLUS_TILE_RES, local_size_z = 1) in;

void main(void)
{
    uint lightCount = POINT_LIGHT_COUNT + SPOT_LIGHT_COUNT;

    ivec2 location = ivec2(gl_GlobalInvocationID.xy);
    ivec2 itemID = ivec2(gl_LocalInvocationID.xy);
    ivec2 tileID = ivec2(gl_WorkGroupID.xy);
    ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
    uint index = tileID.y * tileNumber.x + tileID.x;

    // Initialize shared global values for depth and light count
    if (gl_LocalInvocationIndex == 0) {
        minDepthInt = 0xFFFFFFFF;
        maxDepthInt = 0;
        visibleLightCount = 0;
        viewInverse = inverse(dvd_ViewMatrix);
    }

    barrier();

    // Step 1: Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
    float maxDepth, minDepth;
    vec2 text = vec2(location) / vec2(windowWidth, windowHeight);
    float depth = texture(depthBuffer, text).r;
    // Linearize the depth value from depth buffer (must do this because we created it using projection)
    depth = (0.5 * dvd_ProjectionMatrix[3][2]) / (depth + 0.5 * dvd_ProjectionMatrix[2][2] - 0.5);

    // Convert depth to uint so we can do atomic min and max comparisons between the threads
    uint depthInt = floatBitsToUint(depth);
    atomicMin(minDepthInt, depthInt);
    atomicMax(maxDepthInt, depthInt);

    barrier();

    // Step 2: One thread should calculate the frustum planes to be used for this tile
    if (gl_LocalInvocationIndex == 0) {
        // Convert the min and max across the entire tile back to float
        minDepth = uintBitsToFloat(minDepthInt);
        maxDepth = uintBitsToFloat(maxDepthInt);

        // Steps based on tile sale
        vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileNumber);
        vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileNumber);

        // Set up starting values for planes using steps and min and max z values
        frustumPlanes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negativeStep.x); // Left
        frustumPlanes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positiveStep.x); // Right
        frustumPlanes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negativeStep.y); // Bottom
        frustumPlanes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positiveStep.y); // Top
        frustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth); // Near
        frustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth); // Far

        // Transform the first four planes
        for (uint i = 0; i < 4; i++) {
            frustumPlanes[i] *= dvd_ViewProjectionMatrix;
            frustumPlanes[i] /= length(frustumPlanes[i].xyz);
        }

        // Transform the depth planes
        frustumPlanes[4] *= dvd_ViewMatrix;
        frustumPlanes[4] /= length(frustumPlanes[4].xyz);
        frustumPlanes[5] *= dvd_ViewMatrix;
        frustumPlanes[5] /= length(frustumPlanes[5].xyz);
    }

    barrier();

    // Step 3: Cull lights.
    // Parallelize the threads against the lights now.
    // Can handle 256 simultaniously. Anymore lights than that and additional passes are performed
    uint threadCount = FORWARD_PLUS_TILE_RES * FORWARD_PLUS_TILE_RES;
    uint passCount = (lightCount + threadCount - 1) / threadCount;
    for (uint i = 0; i < passCount; i++) {
        // Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
        uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
        if (lightIndex >= lightCount) {
            break;
        }

        Light source = dvd_LightSource[lightIndex + DIRECTIONAL_LIGHT_COUNT];
        vec4 position = source._positionWV;
        float radius = position.w;
        position.w = 1.0f;

        position = viewInverse * position;

        // We check if the light exists in our frustum
        float distance = 0.0;
        for (uint j = 0; j < 6; ++j) {
            distance = dot(position, frustumPlanes[j]) + radius;

            // If one of the tests fails, then there is no intersection
            if (distance <= 0.0) {
                break;
            }
        }

        // If greater than zero, then it is a visible light
        if (distance > 0.0) {
            // Add index to the shared array of visible indices
            uint offset = atomicAdd(visibleLightCount, 1);
            // Offset spot lights by point light count
            visibleLightIndices[offset] = int(lightIndex + (source._options.x - 1) * POINT_LIGHT_COUNT);
        }
    }

    barrier();

    // One thread should fill the global light buffer
    if (gl_LocalInvocationIndex == 0) {
        uint offset = index * MAX_NUM_LIGHTS_PER_TILE; // Determine bosition in global buffer
        for (uint i = 0; i < visibleLightCount; i++) {
            perTileLightIndices[offset + i] = visibleLightIndices[i];
        }

        if (visibleLightCount != MAX_NUM_LIGHTS_PER_TILE) {
            // Unless we have totally filled the entire array, mark it's end with -1
            // Final shader step will use this to determine where to stop (without having to pass the light count)
            perTileLightIndices[offset + visibleLightCount] = -1;
        }
    }
}