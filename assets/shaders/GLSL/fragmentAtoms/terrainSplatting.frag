#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

layout(binding = TEXTURE_TERRAIN_SPLAT) uniform sampler2DArray texBlendMaps;
#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
layout(binding = TEXTURE_NORMALMAP) uniform sampler2DArray texNormalMaps;
#endif

#if !defined(PRE_PASS)
layout(binding = TEXTURE_UNIT0)  uniform sampler2D texAlbedo;
layout(binding = TEXTURE_TERRAIN_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
#endif

#include "texturing.frag"

#if !defined(PRE_PASS)
vec4 getTerrainAlbedo(in vec2 uv) {
#if defined(LOW_QUALITY)
    return texture(texAlbedo, uv);
#else
    vec4 colour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 detail = vec4(1.0f, 1.0f, 1.0f, 0.0f);

    if (LoD > 1) {
        colour = texture(texAlbedo, uv);
        if (LoD > 2) {
            return colour;
        }
        
        detail = textureNoTile(texTileMaps, vec3(scaledTextureCoords(uv, TEXTURE_TILE_SIZE * DETAIL_TILLING), 1));
    } else {
        uint offset = 0;
        for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
            const vec4 blendColour = texture(texBlendMaps, vec3(uv, i));

            const uint layerCount = CURRENT_LAYER_COUNT[i];
            for (uint j = 0; j < layerCount; ++j) {
                vec3 coordsC = vec3(scaledTextureCoords(uv, TEXTURE_TILE_SIZE), offset + (j * 2) + 0);
                vec3 coordsD = vec3(scaledTextureCoords(uv, TEXTURE_TILE_SIZE * DETAIL_TILLING), offset + (j * 2) + 1);

                float amnt = blendColour[j];
                colour = mix(colour, textureNoTile(texTileMaps, coordsC), amnt);
                detail = mix(detail, textureNoTile(texTileMaps, coordsD), amnt);
            }

            offset += CURRENT_LAYER_COUNT[i] * 2;
        }
    }
    return vec4(colour.rgb /** (detail.rgb * DETAIL_BRIGHTNESS)*/, 1.0f);
#endif
}
#endif //PRE_PASS


#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
#if !defined(LOW_QUALITY)
vec3 _getSplatNormal(in vec2 uv) {
    vec3 tbn = vec3(1.0f);
    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const uint layerCount = CURRENT_LAYER_COUNT[i];

        vec4 blendColour = texture(texBlendMaps, vec3(uv, i));
        for (uint j = 0; j < layerCount; ++j) {
            vec3 scaledCoords = vec3(scaledTextureCoords(uv, TEXTURE_TILE_SIZE), offset + j);
            tbn = mix(tbn, textureNoTile(texNormalMaps, scaledCoords).rgb, blendColour[j]);
        }
        offset += CURRENT_LAYER_COUNT[i];
    }

    return VAR._tbn * normalize(2.0f * tbn - 1.0f);
}
#endif

vec3 getTerrainNormalTBN(in vec2 uv) {
#if defined(LOW_QUALITY)
    return VAR._normalWV;
#else //LOW_QUALITY
    return _getSplatNormal(uv);

#endif //LOW_QUALITY
}
#endif //PRE_PASS

#endif //_TERRAIN_SPLATTING_FRAG_