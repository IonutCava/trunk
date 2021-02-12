--Fragment

#include "utility.frag"

/* sum(rgb * a, a) */
layout(binding = TEXTURE_UNIT0) uniform sampler2D accumTexture;
/* prod(1 - a) */
layout(binding = TEXTURE_UNIT1) uniform sampler2D revealageTexture;

layout(location = TARGET_ALBEDO) out vec4 _colourOut;

void main() {
    ivec2 C = ivec2(gl_FragCoord.xy);

    const vec2 revIn = texelFetch(revealageTexture, C, 0).rg;
    const float revealage = revIn.r;

    if (revealage >= INV_Z_TEST_SIGMA) {
        // Save the blending and color texture fetch cost
        discard;
    }

    vec4 accum = texelFetch(accumTexture, C, 0);
    vec3 averageColor  = accum.rgb / clamp(accum.a, 1e-4f, 5e4f);
    // dst' =  (accum.rgb / accum.a) * (1 - revealage) + dst
    // [dst has already been modulated by the transmission colors and coverage and the blend mode inverts revealage for us] 
    _colourOut = vec4(averageColor, 1.0f - revealage);
}


