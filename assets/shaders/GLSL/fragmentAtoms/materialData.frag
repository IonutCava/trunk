#ifndef _MATERIAL_DATA_FRAG_
#define _MATERIAL_DATA_FRAG_

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP)
    #define HAS_TRANSPARENCY
#endif

#if !defined(SKIP_TEXTURES)
layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texDiffuse1;
#endif
//Normal or BumpMap
#if defined(COMPUTE_TBN)
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texNormalMap;
#endif

//Specular and opacity maps are available even for non-textured geometry
#if defined(USE_OPACITY_MAP)
layout(binding = TEXTURE_OPACITY) uniform sampler2D texOpacityMap;
#endif

#if defined(USE_SPECULAR_MAP)
layout(binding = TEXTURE_SPECULAR) uniform sampler2D texSpecularMap;
#endif

#if defined(USE_REFLECTIVE_CUBEMAP)
layout(binding = TEXTURE_REFLECTION) uniform samplerCubeArray texEnvironmentCube;
#endif

float Gloss(in vec3 bump, in vec2 texCoord)
{
    #if defined(USE_TOKSVIG)
        // Compute the "Toksvig Factor"
        float rlen = 1.0/saturate(length(bump));
        return 1.0/(1.0 + power*(rlen - 1.0));
    #elif defined(USE_TOKSVIG_MAP)
        float baked_power = 100.0;
        // Fetch pre-computed "Toksvig Factor"
        // and adjust for specified power
        float gloss = texture2D(texSpecularMap, texCoord).r;
        gloss /= mix(power/baked_power, 1.0, gloss);
        return gloss;
    #else
        return 1.0;
    #endif
}

#if !defined(SKIP_TEXTURES)
vec4 getTextureColor(in vec2 uv) {
    #define TEX_MODULATE 0
    #define TEX_ADD  1
    #define TEX_SUBTRACT  2
    #define TEX_DIVIDE  3
    #define TEX_SMOOTH_ADD  4
    #define TEX_SIGNED_ADD  5
    #define TEX_DECAL  6
    #define TEX_REPLACE  7

    vec4 color = texture(texDiffuse0, uv);

    if (dvd_textureCount == 1) {
        return color;
    }

    vec4 color2 = texture(texDiffuse1, uv);

    // Read from the texture
    switch (dvd_texOperation) {
        default             : color = vec4(0.7743, 0.3188, 0.5465, 1.0);   break; // <hot pink to easily spot it in a crowd
        case TEX_MODULATE   : color *= color2;       break;
        case TEX_REPLACE    : color  = color2;       break;
        case TEX_SIGNED_ADD : color += color2 - 0.5; break;
        case TEX_DIVIDE     : color /= color2;          break;
        case TEX_SUBTRACT   : color -= color2;          break;
        case TEX_DECAL      : color = vec4(mix(color.rgb, color2.rgb, color2.a), color.a); break;
        case TEX_ADD        : {
            color.rgb += color2.rgb;
            color.a   *= color2.a;
        }break;
        case TEX_SMOOTH_ADD : {
            color = (color + color2) - (color * color2);
        }break;
    }
    

    return saturate(color);
}
#endif

vec4 dvd_MatDiffuse;
vec3 dvd_MatSpecular;
vec3 dvd_MatEmissive;
float dvd_MatShininess;

void parseMaterial() {
    
    dvd_MatEmissive = dvd_Matrices[VAR.dvd_drawID]._colorMatrix[2].rgb;
    dvd_MatShininess = dvd_Matrices[VAR.dvd_drawID]._colorMatrix[2].w;

    #if !defined(USE_CUSTOM_ALBEDO)
        #if defined(SKIP_TEXTURES)
            dvd_MatDiffuse = dvd_Matrices[VAR.dvd_drawID]._colorMatrix[0];
        #else
            dvd_MatDiffuse = getTextureColor(VAR._texCoord);
        #endif
    #endif

    #if defined(USE_SPECULAR_MAP)
        dvd_MatSpecular = texture(texSpecularMap, VAR._texCoord).rgb;
    #else
        dvd_MatSpecular = dvd_Matrices[VAR.dvd_drawID]._colorMatrix[1].rgb;
    #endif
}

#endif //_MATERIAL_DATA_FRAG_