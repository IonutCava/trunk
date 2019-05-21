#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

#include "texturing.frag"

uniform vec4 tileScale[MAX_TEXTURE_LAYERS];

layout(binding = TEXTURE_UNIT0)     uniform sampler2D texWaterCaustics;
layout(binding = TEXTURE_UNIT1)     uniform sampler2D texUnderwaterAlbedo;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texUnderwaterDetail;
layout(binding = TEXTURE_OPACITY)   uniform sampler2D texHeightMap;

layout(binding = TEXTURE_TERRAIN_SPLAT)       uniform sampler2DArray texBlendMaps;
layout(binding = TEXTURE_TERRAIN_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
layout(binding = TEXTURE_TERRAIN_NORMAL_TILE) uniform sampler2DArray texNormalMaps;


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

vec3 getTerrainAlbedoAndNormalTBN(in vec2 uv, out vec4 colour) {
    colour = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    vec3 tbn = vec3(1.0f);

    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const uint layerCount = CURRENT_LAYER_COUNT[i];

        vec3 tbnTemp = tbn;
        vec4 texScale = tileScale[i];// / (LoD + 1);
        vec4 blendColour = texture(texBlendMaps, vec3(uv, i));


        uint j = 0;
        for (j = 0; j < layerCount; ++j) {
            vec3 scaledCoords = vec3(scaledTextureCoords(uv, texScale[j]), offset + j);
            colour = mix(colour, textureNoTile(texTileMaps, scaledCoords), blendColour[j]);
        }
        for (j = 0; j < layerCount; ++j) {
            vec3 scaledCoords = vec3(scaledTextureCoords(uv, texScale[j]), offset + j);
            tbn = mix(tbn, textureNoTile(texNormalMaps, scaledCoords).rgb, blendColour[j]);
        }
        //tbn = normalUDNBlend(tbnTemp, tbn);
        //tbn = tbnTemp;
        offset += CURRENT_LAYER_COUNT[i];
    }

    return normalize(2.0f * tbn - 1.0f);
}

#endif //_TERRAIN_SPLATTING_FRAG_