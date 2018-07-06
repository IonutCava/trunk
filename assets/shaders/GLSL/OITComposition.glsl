--Vertex

void main(void)
{
    vec2 uv = vec2(0, 0);
    if ((gl_VertexID & 1) != 0) {
        uv.x = 1;
    }

    if ((gl_VertexID & 2) != 0) {
        uv.y = 1;
    }

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0, 1);
}

--Fragment

#include "Utility.frag"

/* sum(rgb * a, a) */
layout(binding = TEXTURE_UNIT0) uniform sampler2D accumTexture;
/* prod(1 - a) */
layout(binding = TEXTURE_UNIT1) uniform sampler2D revealageTexture;

out vec4 result;

void main() {
    ivec2 C = ivec2(gl_FragCoord.xy);

    float revealage = texelFetch(revealageTexture, C, 0).r;
    if (revealage == 1.0) {
        // Save the blending and color texture fetch cost
        discard;
    }

    vec4 accum = texelFetch(accumTexture, C, 0);

    // Suppress overflow
    if (isinf(maxComponent(abs(accum)))) {
        accum.rgb = vec3(accum.a);
    }

    // dst' =  (accum.rgb / accum.a) * (1 - revealage) + dst
    // [dst has already been modulated by the transmission colors and coverage and the blend mode
    // inverts revealage for us] 
    result = vec4(accum.rgb / max(accum.a, 0.00001), revealage);
}


