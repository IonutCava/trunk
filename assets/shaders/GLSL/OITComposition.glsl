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

#ifndef USE_COLOURED_WOIT
//#define USE_COLOURED_WOIT
#endif //USE_COLOURED_WOIT


#include "Utility.frag"

/* sum(rgb * a, a) */
layout(binding = TEXTURE_UNIT0) uniform sampler2D accumTexture;
/* prod(1 - a) */
layout(binding = TEXTURE_UNIT1) uniform sampler2D revealageTexture;

//layout(binding = TEXTURE_NORMALMAP) uniform sampler2D normalAndVelocity;

layout(location = 0) out vec4 _colourOut;
//layout(location = 1) out vec4 _normalAndVelocityOut;

void main() {
    ivec2 C = ivec2(gl_FragCoord.xy);

    float revealage = texelFetch(revealageTexture, C, 0).r;
    if (revealage == 1.0f) {
        // Save the blending and color texture fetch cost
        discard;
    }

    vec4 accum = texelFetch(accumTexture, C, 0);
    //vec4 normalData = texelFetch(normalAndVelocity, C, 0);
    // Suppress overflow
    if (isinf(maxComponent(abs(accum)))) {
        accum.rgb = vec3(accum.a);
    }

    // dst' =  (accum.rgb / accum.a) * (1 - revealage) + dst
    // [dst has already been modulated by the transmission colors and coverage and the blend mode
    // inverts revealage for us] 
    vec3 averageColor = accum.rgb / max(accum.a, 0.00001f);
#ifdef USE_COLOURED_WOIT
    _colourOut = vec4(averageColor, revealage);
#else
    _colourOut = vec4(averageColor, 1.0f - revealage);
#endif
    //_normalAndVelocityOut = normalData;
}


