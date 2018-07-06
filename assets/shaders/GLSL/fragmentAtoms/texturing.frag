layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texDiffuse1;

vec4 getTextureColor(in vec2 uv) {
    #define TEX_MODULATE 0
    #define TEX_ADD  1
    #define TEX_SUBSTRACT  2
    #define TEX_DIVIDE  3
    #define TEX_SMOOTH_ADD  4
    #define TEX_SIGNED_ADD  5
    #define TEX_DECAL  6
    #define TEX_REPLACE  7

    vec4 color = texture(texDiffuse0, uv);

    if(dvd_TextureCount == 1)
        return color;

    vec4 color2 = texture(texDiffuse1, uv);

    // Read from the texture
    switch (dvd_texOperation) {
        default             : color = vec4(0.7743, 0.3188, 0.5465, 1.0);   break; // <hot pink to easily spot it in a crowd
        case TEX_MODULATE   : color *= color2;       break;
        case TEX_REPLACE    : color  = color2;       break;
        case TEX_SIGNED_ADD : color += color2 - 0.5; break;
        case TEX_DIVIDE     : color /= color2; 	     break;
        case TEX_SUBSTRACT  : color -= color2; 	     break;
        case TEX_DECAL      : color = vec4(mix(color.rgb, color2.rgb, color2.a), color.a); break;
        case TEX_ADD        : {
            color.rgb += color2.rgb;
            color.a   *= color2.a;
        }break;
        case TEX_SMOOTH_ADD : {
            color = (color + color2) - (color * color2);
        }break;
    }
    

    return clamp(color, 0.0, 1.0);
}
