-- Compute

layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D depthTex;
layout(binding = TEXTURE_DEPTH_MAP_PREV) uniform sampler2D prevDepthTex;
layout(binding = TEXTURE_UNIT1, rg16) writeonly uniform image2D velocityTex;

// we use 16 * 16 threads groups
layout(local_size_x = WORK_GROUP_SIZE, local_size_y = WORK_GROUP_SIZE) in;

void main(void)
{

    ivec2 position = ivec2(gl_GlobalInvocationID.xy);
    vec2  screenNormalized = vec2(position) * vec2(dvd_invScreenDimensions);

    float crtDepth = textureLod(depthTex, screenNormalized, 0).r;
    float prevDepth = textureLod(prevDepthTex, screenNormalized, 0).r;

    vec4 crtPos = positionFromDepth(crtDepth, dvd_InvProjectionMatrix, screenNormalized);
    vec4 prevPos = positionFromDepth(prevDepth, dvd_InvProjectionMatrix, screenNormalized);

    vec4 velocity = (crtPos - prevPos) * 0.5f;
    imageStore(velocityTex, position, velocity);
}