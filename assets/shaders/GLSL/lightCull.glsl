--Compute

//ToDo: change to this: https://github.com/bcrusco/Forward-Plus-Renderer/blob/master/Forward-Plus/Forward-Plus/source/shaders/light_culling.comp.glsl
#include "lightInput.cmn"

#define DIRECTIONAL_LIGHT_COUNT dvd_LightData.x
#define POINT_LIGHT_COUNT       dvd_LightData.y
#define SPOT_LIGHT_COUNT        dvd_LightData.z

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;
uniform mat4 viewProjectionMatrix;
uniform vec2 viewportDimensions;

layout(binding = TEXTURE_DEPTH_MAP, r32f) readonly uniform image2D HiZBuffer;

shared uint min_depth;
shared uint max_depth;
shared vec4 frustum_planes[6];
shared uint group_light_count;
shared int group_light_index[MAX_LIGHTS_PER_TILE];

layout(local_size_x = FORWARD_PLUS_TILE_RES, local_size_y = FORWARD_PLUS_TILE_RES, local_size_z = 1) in;

void main(void)
{
    ivec2 tile_id = ivec2(gl_WorkGroupID.xy);
    if (gl_LocalInvocationIndex == 0) {
        min_depth = 0xFFFFFFFu;
        max_depth = 0x0;
        group_light_count = 0;
    }
    barrier();

    // Step 1: Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
    ivec2 screen_uv = ivec2(gl_GlobalInvocationID.xy / viewportDimensions);

    float depth = imageLoad(HiZBuffer, screen_uv).r;

    uint depth_uint = floatBitsToUint(depth);
    atomicMin(min_depth, depth_uint);
    atomicMax(max_depth, depth_uint);

    barrier();

    // Step 2: One thread should calculate the frustum planes to be used for this tile
    if (gl_LocalInvocationIndex == 0) {

        float min_group_depth = 2.0f * uintBitsToFloat(min_depth) - 1.0f;
        float max_group_depth = 2.0f * uintBitsToFloat(max_depth) - 1.0f;

        vec4 vs_min_depth = (invProjectionMatrix * vec4(0.0f, 0.0f, min_group_depth, 1.0f));
        vec4 vs_max_depth = (invProjectionMatrix * vec4(0.0f, 0.0f, max_group_depth, 1.0f));
        vs_min_depth /= vs_min_depth.w;
        vs_max_depth /= vs_max_depth.w;
        min_group_depth = vs_min_depth.z;
        max_group_depth = vs_max_depth.z;

        vec2 tile_scale = viewportDimensions * (1.0f / (2.0f * FORWARD_PLUS_TILE_RES));
        vec2 tile_bias = tile_scale - vec2(gl_WorkGroupID.xy);

        vec4 col1 = vec4(-projectionMatrix[0][0] * tile_scale.x,  projectionMatrix[0][1],                tile_bias.x, projectionMatrix[0][3]);
        vec4 col2 = vec4( projectionMatrix[1][0],                -projectionMatrix[1][1] * tile_scale.y, tile_bias.y, projectionMatrix[1][3]);
        vec4 col4 = vec4( projectionMatrix[3][0],                 projectionMatrix[3][1],                -1.0,        projectionMatrix[3][3]);

        frustum_planes[0] = col4 + col1;
        frustum_planes[1] = col4 - col1;
        frustum_planes[2] = col4 - col2;
        frustum_planes[3] = col4 + col2;
        frustum_planes[4] = vec4(0.0, 0.0,  1.0, -min_group_depth);
        frustum_planes[5] = vec4(0.0, 0.0, -1.0,  max_group_depth);
        for (uint i = 0; i < 4; i++) {
            frustum_planes[i] *= 1.0f / length(frustum_planes[i].xyz);
        }

    }

    barrier();

    // Step 3: Cull lights.
    uint lightCount = POINT_LIGHT_COUNT + SPOT_LIGHT_COUNT;
    uint thread_count = FORWARD_PLUS_TILE_RES * FORWARD_PLUS_TILE_RES;

    for (uint i = gl_LocalInvocationIndex; i < lightCount; i += thread_count) {
        Light source = dvd_LightSource[i + DIRECTIONAL_LIGHT_COUNT];
        vec4 vs_light_pos = source._positionWV;
        float radius = vs_light_pos.w;
        vs_light_pos.w = 1.0f;
        if (group_light_count < MAX_LIGHTS_PER_TILE) {
            bool inFrustum = true;
            for (uint j = 0; j < 4 && inFrustum; ++j) {
                float d = dot(frustum_planes[j], vs_light_pos);
                inFrustum = (d >= -radius);
            }
            if (inFrustum) {
                uint id = atomicAdd(group_light_count, 1);
                group_light_index[id] = int(i);
            }
        }
    }

    barrier();

    // One thread should fill the global light buffer
    if (gl_LocalInvocationIndex == 0) {
        uint index = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;
        uint offset = index * MAX_LIGHTS_PER_TILE;

        for(uint i = 0; i < group_light_count; ++i) {
            perTileLightIndices[offset + i] = group_light_index[i];
        }

        if (group_light_count != MAX_LIGHTS_PER_TILE) {
            // Unless we have totally filled the entire array, mark it's end with -1
            // Final shader step will use this to determine where to stop (without having to pass the light count)
            perTileLightIndices[offset + group_light_count] = -1;
        }
    }
}