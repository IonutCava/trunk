--Compute

//Most of the stuff is from here: https://github.com/WindyDarian/Vulkan-Forward-Plus-Renderer/blob/master/src/shaders/light_culling.comp.glsl
//Mixed with: https://github.com/bcrusco/Forward-Plus-Renderer/blob/master/Forward-Plus/Forward-Plus/source/shaders/light_culling.comp.glsl

#include "lightInput.cmn"

#define DIRECTIONAL_LIGHT_COUNT dvd_LightData.x
#define POINT_LIGHT_COUNT       dvd_LightData.y
#define SPOT_LIGHT_COUNT        dvd_LightData.z

struct ViewFrustum
{
    vec4 planes[6];
    vec3 points[8]; // 0-3 near 4-7 far
};

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewProjectionMatrix;
uniform vec2 viewportDimensions;

#if defined(USE_IMAGE_LOAD_STORE)
layout(binding = TEXTURE_DEPTH_MAP, r32f) readonly uniform image2D HiZBuffer;
#else
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D HiZBuffer;
#endif

shared ViewFrustum frustum;
shared uint light_count_for_tile;
shared int group_light_index[MAX_LIGHTS_PER_TILE];
shared uint minDepthInt;
shared uint maxDepthInt;
shared mat4 invViewMatrix;

layout(local_size_x = FORWARD_PLUS_TILE_RES, local_size_y = FORWARD_PLUS_TILE_RES, local_size_z = 1) in;

vec3 IntersectPlanes(vec4 P0, vec4 P1, vec4 P2) {
    vec3 bxc = cross(P1.xyz, P2.xyz);
    vec3 cxa = cross(P2.xyz, P0.xyz);
    vec3 axb = cross(P0.xyz, P1.xyz);
    vec3 r = -P0.w * bxc - P1.w * cxa - P2.w * axb;
    return r * (1 / dot(P0.xyz, bxc));
}

// Construct view frustum
ViewFrustum CreateFrustum(ivec2 tile_id, float min_depth, float max_depth) {
#   define LEFT 0
#   define RIGHT 1
#   define BOTTOM 2
#   define TOP 3
#   define NEAR 4
#   define FAR 5

    ViewFrustum frustum;

    ivec2 tile_number = ivec2(gl_NumWorkGroups.xy);
    vec2 negativeStep = (2.0f * vec2(tile_id)) / vec2(tile_number);
    vec2 positiveStep = (2.0f * vec2(tile_id + ivec2(1, 1))) / vec2(tile_number);

    // Set up starting values for planes using steps and min and max z values
    frustum.planes[LEFT]   = vec4( 1.0,  0.0,  0.0,  1.0 - negativeStep.x);
    frustum.planes[RIGHT]  = vec4(-1.0,  0.0,  0.0, -1.0 + positiveStep.x);
    frustum.planes[BOTTOM] = vec4( 0.0,  1.0,  0.0,  1.0 - negativeStep.y);
    frustum.planes[TOP]    = vec4( 0.0, -1.0,  0.0, -1.0 + positiveStep.y);
    frustum.planes[NEAR]   = vec4( 0.0,  0.0, -1.0, -min_depth);
    frustum.planes[FAR]    = vec4( 0.0,  0.0,  1.0,  max_depth);

    // Transform the first four planes
    for (uint i = 0; i < 4; i++) {
        frustum.planes[i] *= viewProjectionMatrix;
        frustum.planes[i] /= length(frustum.planes[i].xyz);
        frustum.planes[i] *= invViewMatrix;
    }

    // Transform the depth planes
    frustum.planes[NEAR] *= viewMatrix;
    frustum.planes[NEAR] /= length(frustum.planes[NEAR].xyz);
    frustum.planes[NEAR] *= invViewMatrix;

    frustum.planes[FAR]  *= viewMatrix;
    frustum.planes[FAR]  /= length(frustum.planes[FAR].xyz);
    frustum.planes[FAR]  *= invViewMatrix;

    frustum.points[0] = IntersectPlanes(frustum.planes[NEAR], frustum.planes[LEFT], frustum.planes[BOTTOM]);
    frustum.points[1] = IntersectPlanes(frustum.planes[NEAR], frustum.planes[LEFT], frustum.planes[TOP]);
    frustum.points[2] = IntersectPlanes(frustum.planes[NEAR], frustum.planes[RIGHT], frustum.planes[TOP]);
    frustum.points[3] = IntersectPlanes(frustum.planes[NEAR], frustum.planes[RIGHT], frustum.planes[BOTTOM]);
    frustum.points[4] = IntersectPlanes(frustum.planes[FAR], frustum.planes[LEFT], frustum.planes[BOTTOM]);
    frustum.points[5] = IntersectPlanes(frustum.planes[FAR], frustum.planes[LEFT], frustum.planes[TOP]);
    frustum.points[6] = IntersectPlanes(frustum.planes[FAR], frustum.planes[RIGHT], frustum.planes[TOP]);
    frustum.points[7] = IntersectPlanes(frustum.planes[FAR], frustum.planes[RIGHT], frustum.planes[BOTTOM]);

    return frustum;
}

bool IsCollided(vec3 pos, float radius, ViewFrustum frustum) {
    // Step1: sphere-plane test
    for (int i = 0; i < 6; i++) {
        vec4 plane = frustum.planes[i];
        if (dot(pos, plane.xyz) + plane.w < -radius) {
            return false;
        }
    }

    // Step2: bbox corner test (to reduce false positive)
    vec3 light_bbox_max = pos + vec3(radius);
    vec3 light_bbox_min = pos - vec3(radius);
    int probe;
    probe = 0; for (int i = 0; i < 8; i++) probe += ((frustum.points[i].x > light_bbox_max.x) ? 1 : 0); if (probe == 8) return false;
    probe = 0; for (int i = 0; i < 8; i++) probe += ((frustum.points[i].x < light_bbox_min.x) ? 1 : 0); if (probe == 8) return false;
    probe = 0; for (int i = 0; i < 8; i++) probe += ((frustum.points[i].y > light_bbox_max.y) ? 1 : 0); if (probe == 8) return false;
    probe = 0; for (int i = 0; i < 8; i++) probe += ((frustum.points[i].y < light_bbox_min.y) ? 1 : 0); if (probe == 8) return false;
    probe = 0; for (int i = 0; i < 8; i++) probe += ((frustum.points[i].z > light_bbox_max.z) ? 1 : 0); if (probe == 8) return false;
    probe = 0; for (int i = 0; i < 8; i++) probe += ((frustum.points[i].z < light_bbox_min.z) ? 1 : 0); if (probe == 8) return false;
    
    return true;
}

void main(void)
{
    ivec2 tile_id = ivec2(gl_WorkGroupID.xy);

    if (gl_LocalInvocationIndex == 0) {
        invViewMatrix = inverse(viewMatrix);
        minDepthInt = 0xFFFFFFFu;
        maxDepthInt = 0;
        light_count_for_tile = 0;
    }

    barrier();

    // Step 1: Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
    vec2 text = gl_GlobalInvocationID.xy / viewportDimensions;
#if defined(USE_IMAGE_LOAD_STORE)
    float depth = imageLoad(HiZBuffer, ivec2(text)).r;
#else
    float depth = texture(HiZBuffer, text).r;
#endif
    // Linearize the depth value from depth buffer (must do this because we created it using projection)
    depth = (projectionMatrix[3][2]) / (depth + projectionMatrix[2][2]);

    // Convert depth to uint so we can do atomic min and max comparisons between the threads
    uint depthInt = floatBitsToUint(depth);
    atomicMin(minDepthInt, depthInt);
    atomicMax(maxDepthInt, depthInt);

    barrier();

    // Step 2: One thread should calculate the frustum planes to be used for this tile
    if (gl_LocalInvocationIndex == 0) {
        // Convert the min and max across the entire tile back to float when calculating the tile frustum
        frustum = CreateFrustum(tile_id,
                                uintBitsToFloat(minDepthInt),
                                uintBitsToFloat(maxDepthInt));
    }

    barrier();

    const uint lightCount = POINT_LIGHT_COUNT + SPOT_LIGHT_COUNT;
    const uint workGroupSizeX = gl_WorkGroupSize.x;

    for (uint i = gl_LocalInvocationIndex; i < lightCount; i += workGroupSizeX)
    {
        Light source = dvd_LightSource[i + DIRECTIONAL_LIGHT_COUNT];
        vec4 vs_light_pos = source._positionWV;
        float range = vs_light_pos.w;
        if (source._options.x == 2) {
            //spot lights are a bit different. Create a bounding sphere by offsetting the center by half the range in the spot direction
            range = source._options.z * 0.5f; //range to radius conversion
            vs_light_pos.xyz += source._directionWV.xyz * range;
        }

        if (IsCollided(vs_light_pos.xyz, range, frustum)) {
            uint slot = atomicAdd(light_count_for_tile, 1);
            if (slot >= MAX_LIGHTS_PER_TILE) {
                break; 
            }
            group_light_index[slot] = int(i);
        }
    }

    barrier();

    if (gl_LocalInvocationIndex == 0) {
        uint tile_index = tile_id.y * gl_NumWorkGroups.x + tile_id.x;
        uint offset = tile_index * MAX_LIGHTS_PER_TILE;

        for (uint i = 0; i < light_count_for_tile; ++i) {
            perTileLightIndices[offset + i] = group_light_index[i];
        }

        if (light_count_for_tile != MAX_LIGHTS_PER_TILE) {
            // Unless we have totally filled the entire array, mark it's end with -1
            // Final shader step will use this to determine where to stop (without having to pass the light count)
            perTileLightIndices[offset + light_count_for_tile] = -1;
        }
    }
}