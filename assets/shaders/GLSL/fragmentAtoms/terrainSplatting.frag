#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

#include "bumpMapping.frag"

uniform vec4 tileScale[MAX_TEXTURE_LAYERS];

layout(binding = TEXTURE_UNIT0)     uniform sampler2D texWaterCaustics;
layout(binding = TEXTURE_UNIT1)     uniform sampler2D texUnderwaterAlbedo;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texUnderwaterDetail;
layout(binding = TEXTURE_OPACITY)   uniform sampler2D texHeightMap;

layout(binding = TEXTURE_COUNT + 0) uniform sampler2DArray texBlendMaps;
layout(binding = TEXTURE_COUNT + 1) uniform sampler2DArray texTileMaps;
layout(binding = TEXTURE_COUNT + 2) uniform sampler2DArray texNormalMaps;

vec4 getTerrainAlbedo(){
    vec4 colour = vec4(0.0, 0.0, 0.0, 1.0);

    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        for (uint j = 0; j < CURRENT_LAYER_COUNT[i]; ++j) {
            colour = mix(colour,
                         texture(texTileMaps, vec3(scaledTextureCoords(VAR._texCoord, tileScale[i][j]), offset + j)),
                         texture(texBlendMaps, vec3(VAR._texCoord, i))[j]);
        }

        offset += CURRENT_LAYER_COUNT[i];
    }

    return colour;
}

vec3 getTerrainNormalTBN() {
    vec3 tbn = vec3(1.0);

    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        vec3 tbnTemp = tbn;
        for (uint j = 0; j < CURRENT_LAYER_COUNT[i]; ++j) {
            tbnTemp = mix(tbnTemp,
                          texture(texNormalMaps, vec3(scaledTextureCoords(VAR._texCoord, tileScale[i][j]), offset + j)).rgb,
                          texture(texBlendMaps, vec3(VAR._texCoord, i))[j]);
        }
        //tbn = normalUDNBlend(tbnTemp, tbn);
        tbn = tbnTemp;
        offset += CURRENT_LAYER_COUNT[i];
    }

    return normalize(2.0 * tbn - 1.0);
}

#endif //_TERRAIN_SPLATTING_FRAG_