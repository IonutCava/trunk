//Textures
//0 -> no texture
//1 -> use texDiffuse0
//n -> add texDiffuse0 + texDiffuse1 + ... texDiffuse(n-1)
uniform int textureCount;

uniform sampler2D texDiffuse[3];
uniform int  textureOperation[3];

vec4 getTextureColor(in vec2 uv) {
    #define TEX_MODULATE 0
    #define TEX_ADD  1
    #define TEX_SUBSTRACT  2
    #define TEX_DIVIDE  3
    #define TEX_SMOOTH_ADD  4
    #define TEX_SIGNED_ADD  5
    #define TEX_DECAL  6
    #define TEX_REPLACE  7

    vec4 color = vec4(0.0);
    for (uint i = 0; i < 3; ++i){
        if (textureCount == i) break;

        // Read from the texture
        switch (textureOperation[i]) {
            default             : color = vec4(0.7743, 0.3188, 0.5465, 1.0);   break; // <hot pink to easily spot
            case TEX_MODULATE   : color *= texture(texDiffuse[i], uv);         break;
            case TEX_REPLACE    : color = texture(texDiffuse[i], uv);          break;
            case TEX_SIGNED_ADD : color += (texture(texDiffuse[i], uv) - 0.5); break;
            case TEX_DIVIDE     : color /= texture(texDiffuse[i], uv); 	       break;
            case TEX_SUBSTRACT  : color -= texture(texDiffuse[i], uv); 	       break;
            case TEX_DECAL      : {
                vec4 temp = texture(texDiffuse[i], uv);
                color = vec4(mix(color.rgb, temp.rgb, temp.a), color.a);
            }break;
            case TEX_ADD        : {
                vec4 tex = texture(texDiffuse[i], uv);
                color.rgb += tex.rgb;
                color.a   *= tex.a;
            }break;
            case TEX_SMOOTH_ADD : {
                vec4 tex = texture(texDiffuse[i], uv);
                color = (color + tex) - (color * tex);
            }break;
        }
    }

    return clamp(color, 0.0, 1.0);
}
