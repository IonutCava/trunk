-- Vertex

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0)uv.x = 1;
    if((gl_VertexID & 2) != 0)uv.y = 1;

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}

-- Fragment.SSAOCalc

// Input screen texture
layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texDepth;
 
layout(location = 0) out vec4 _ssaaOut;
layout(location = 1) out vec4 _screenOut;

const float maxz = 0.00035;
const float div_factor = 3.0 ;
const float maxz2 =  maxz * div_factor;
 
float LinearizeDepth(in float z) {
#if defined(USE_SCENE_ZPLANES)
    float n = dvd_ZPlanesCombined.z;
    float f = dvd_ZPlanesCombined.w * 0.5;
#else
    float n = dvd_ZPlanesCombined.x;
    float f = dvd_ZPlanesCombined.y * 0.5;
#endif

    return (2 * n) / (f + n - (z)* (f - n));
}
 
float CompareDepth(in float crtDepth, in vec2 uv1) {
    float dif1 = crtDepth - LinearizeDepth(texture(texDepth, uv1 ).x);
    return clamp(dif1, -maxz, maxz) -  clamp(dif1/div_factor,0.0,maxz2);
}


void main(void) {
    vec2 UV = VAR._texCoord + vec2(0.0011);
    float original_pix = LinearizeDepth(texture(texDepth, UV).x);
    float crtRealDepth = original_pix * dvd_ZPlanesCombined.y + dvd_ZPlanesCombined.x;

    float increment = 0.013 - clamp(original_pix * 0.4, 0.001, 0.009);

    float dif1 = 0.0;
    float dif2 = 0.0;
    float dif3 = 0.0;
    float dif4 = 0.0;
    float dif5 = 0.0;
    float dif6 = 0.0;
    float dif7 = 0.0;
    float dif8 = 0.0;
    float dif = 0.0;

    float increment2 = increment * 0.75;
    vec2 inc1 = vec2(increment2, 0.0);
    vec2 inc2 = vec2(increment2 * 0.7, -increment2 * 0.9);
    vec2 inc3 = vec2(increment2  *0.7, increment2 * 0.9);

    dif1 = CompareDepth(original_pix, UV - inc1);
    dif2 = CompareDepth(original_pix, UV + inc1);
    dif3 = CompareDepth(original_pix, UV + inc2);
    dif4 = CompareDepth(original_pix, UV - inc2);
    dif5 = CompareDepth(original_pix, UV - inc3);
    dif6 = CompareDepth(original_pix, UV + inc3);
    dif1 += dif2;
    dif3 += dif4;
    dif5 += dif6;
    dif = max(dif1, 0.0) + max(dif3, 0.0) + max(dif5, 0.0);

    increment2 += increment;
    inc1 = vec2(increment2, 0.0);
    inc2 = vec2(increment2 * 0.7, -increment2 * 0.9);
    inc3 = vec2(increment2 * 0.7, increment2 * 0.9);
    dif1 = CompareDepth(original_pix, UV - inc1);
    dif2 = CompareDepth(original_pix, UV + inc1);
    dif3 = CompareDepth(original_pix, UV + inc2);
    dif4 = CompareDepth(original_pix, UV - inc2);
    dif5 = CompareDepth(original_pix, UV - inc3);
    dif6 = CompareDepth(original_pix, UV + inc3);
    dif1 += dif2;
    dif3 += dif4;
    dif5 += dif6;
    dif += (max(dif1, 0.0) + max(dif3, 0.0) + max(dif5, 0.0)) * 0.7;

    increment2 += increment * 1.25;
    inc1 = vec2(increment2, 0.0);
    inc2 = vec2(increment2 * 0.7, -increment2 * 0.9);
    inc3 = vec2(increment2 * 0.7, increment2 * 0.9);
    dif1 = CompareDepth(original_pix, UV - inc1);
    dif2 = CompareDepth(original_pix, UV + inc1);
    dif3 = CompareDepth(original_pix, UV + inc2);
    dif4 = CompareDepth(original_pix, UV - inc2);
    dif5 = CompareDepth(original_pix, UV - inc3);
    dif6 = CompareDepth(original_pix, UV + inc3);
    dif1 += dif2;
    dif3 += dif4;
    dif5 += dif6;
    dif += (max(dif1, 0.0) + max(dif3, 0.0) + max(dif5, 0.0)) * 0.5;

    _ssaaOut = vec4(vec3(1.0 - dif * (dvd_ZPlanesCombined.y + dvd_ZPlanesCombined.x) * 255), 1.0);
    _screenOut = texture(texScreen, VAR._texCoord);
}

--Fragment.SSAOApply

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texSSAO;

out vec4 _colorOut;

void main() {
    float ssaoFilter = texture(texSSAO, VAR._texCoord).r;
    _colorOut = texture(texScreen, VAR._texCoord);

    if (ssaoFilter > 0) {
        _colorOut.rgb = _colorOut.rgb * ssaoFilter;
    }
}