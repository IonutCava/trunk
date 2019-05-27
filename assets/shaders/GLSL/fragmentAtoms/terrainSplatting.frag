#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

uniform vec4 tileScale[MAX_TEXTURE_LAYERS];

layout(binding = TEXTURE_TERRAIN_SPLAT) uniform sampler2DArray texBlendMaps;
#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
layout(binding = TEXTURE_NORMALMAP) uniform sampler2DArray texNormalMaps;
#endif

#if !defined(PRE_PASS)
layout(binding = TEXTURE_TERRAIN_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
#endif

#include "texturing.frag"

#if !defined(PRE_PASS)
vec4 getTerrainAlbedo(in vec2 uv) {
    vec4 colour = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        vec4 blendColour = texture(texBlendMaps, vec3(uv, i));
        vec4 texScale = tileScale[i];// / (LoD + 1);
        for (uint j = 0; j < CURRENT_LAYER_COUNT[i]; ++j) {
            colour = mix(colour,
                         textureNoTile(texTileMaps, vec3(scaledTextureCoords(uv, texScale[j]), offset + j)),
                         blendColour[j]);
        }

        offset += CURRENT_LAYER_COUNT[i];
    }

    return colour;
}
#endif //PRE_PASS


#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
vec3 getTerrainNormalTBN(in vec2 uv) {
#if defined(LOW_QUALITY)
    return VAR._normalWV;
#else //LOW_QUALITY
    vec3 tbn = vec3(1.0f);
    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const uint layerCount = CURRENT_LAYER_COUNT[i];

        vec4 texScale = tileScale[i];// / (LoD + 1);
        vec4 blendColour = texture(texBlendMaps, vec3(uv, i));

        uint j = 0;
        for (j = 0; j < layerCount; ++j) {
            vec3 scaledCoords = vec3(scaledTextureCoords(uv, texScale[j]), offset + j);
            tbn = mix(tbn, textureNoTile(texNormalMaps, scaledCoords).rgb, blendColour[j]);
        }
        offset += CURRENT_LAYER_COUNT[i];
    }

    return normalize(2.0f * tbn - 1.0f);
#endif //LOW_QUALITY
}
#endif //PRE_PASS

#endif //_TERRAIN_SPLATTING_FRAG_