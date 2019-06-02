#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

//#define TRI_PLANAR_BLEND

layout(binding = TEXTURE_TERRAIN_SPLAT) uniform sampler2DArray texBlendMaps;
#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
layout(binding = TEXTURE_NORMALMAP) uniform sampler2DArray texNormalMaps;
#endif

#if !defined(PRE_PASS)
layout(binding = TEXTURE_UNIT0)  uniform sampler2D texAlbedo;
layout(binding = TEXTURE_TERRAIN_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
#else
layout(binding = TEXTURE_SPECULAR) uniform sampler2D texNormals;
#endif

#include "texturing.frag"

const int tiling[] = {
    int(TEXTURE_TILE_SIZE),
    int(TEXTURE_TILE_SIZE * 0.25f),
    int(TEXTURE_TILE_SIZE * 0.0625f),
    int(TEXTURE_TILE_SIZE * 0.015625f),
    int(TEXTURE_TILE_SIZE * 0.00390625f),
    int(TEXTURE_TILE_SIZE * 0.0009765625f),
};

vec4 _getTexture(in sampler2DArray tex, in vec3 coords) {
    if (LoD > 1) {
        return textureNoTile(tex, coords);
    }
    return texture(tex, coords);
}

#if !defined(PRE_PASS)

vec4 getTerrainAlbedo(in vec2 uv) {
#if defined(LOW_QUALITY)
    return texture(texAlbedo, uv);
#else

    vec4 colour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 detail = vec4(1.0f, 1.0f, 1.0f, 0.0f);

    const int tileScale = tiling[LoD];

    if (LoD > 2) {
        colour = texture(texAlbedo, uv);
        detail = _getTexture(texTileMaps, vec3(scaledTextureCoords(uv, tileScale * DETAIL_TILLING), 1));
    } else {

        uint offset = 0;
        for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
            const vec4 blendColour = texture(texBlendMaps, vec3(uv, i));

            const uint layerCount = CURRENT_LAYER_COUNT[i];
            for (uint j = 0; j < layerCount; ++j) {
                vec3 coordsC = vec3(scaledTextureCoords(uv, tileScale), offset + (j * 2) + 0);
                vec3 coordsD = vec3(scaledTextureCoords(uv, (tileScale * DETAIL_TILLING)), offset + (j * 2) + 1);

                float amnt = blendColour[j];
                colour = mix(colour, _getTexture(texTileMaps, coordsC), amnt);
                detail = mix(detail, _getTexture(texTileMaps, coordsD), amnt);
            }

            offset += CURRENT_LAYER_COUNT[i] * 2;
        }
    }
    return vec4(colour.rgb * (detail.rgb * DETAIL_BRIGHTNESS), 1.0f);
#endif
}
#endif //PRE_PASS


#if defined(PRE_PASS)
vec3 _getTexNormal(in vec2 uv) {
#if defined(TRI_PLANAR_BLEND)
    const float normalScale = 1.0f;
    const float normalRepeat = 1.0f;
    vec3 blending = _getTriPlanarBlend(inverse(mat3(dvd_ViewMatrix)) * VAR._normalWV);
    vec3 xaxis = texture(texNormals, VAR._vertexW.yz * normalRepeat).rgb;
    vec3 yaxis = texture(texNormals, VAR._vertexW.xz * normalRepeat).rgb;
    vec3 zaxis = texture(texNormals, VAR._vertexW.xy * normalRepeat).rgb;
    vec3 normal = xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;
    normal = 2.0f * normal - 1.0f;
    normal.xy *= normalScale;
    return normalize(normal);
#else
    return normalize(2.0f * texture(texNormals, uv).rgb - 1.0f);
#endif
}

#if !defined(LOW_QUALITY)

#if defined(TRI_PLANAR_BLEND)
vec3 _getTriPlanarBlend(vec3 _wNorm) {
    // in wNorm is the world-space normal of the fragment
    vec3 blending = abs(_wNorm);
    blending = normalize(max(blending, 0.00001)); // Force weights to sum to 1.0
    float b = (blending.x + blending.y + blending.z);
    blending /= vec3(b, b, b);
    return blending;
}
#endif //TRI_PLANAR_BLEND

vec3 _getSplatNormal(in vec2 uv) {
    const int tileScale = tiling[LoD];

    vec3 tbn = vec3(1.0f);
    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const uint layerCount = CURRENT_LAYER_COUNT[i];

        vec4 blendColour = texture(texBlendMaps, vec3(uv, i));
        for (uint j = 0; j < layerCount; ++j) {
            vec3 scaledCoords = vec3(scaledTextureCoords(uv, tileScale), offset + j);
            tbn = mix(tbn, _getTexture(texNormalMaps, scaledCoords).rgb, blendColour[j]);
        }
        offset += CURRENT_LAYER_COUNT[i];
    }

    return normalize(2.0f * tbn - 1.0f);
}
#endif //LOW_QUALITY

vec3 TerrainNormal(in vec2 uv) {
#if defined(LOW_QUALITY)
    return _getTexNormal(uv);
#else //LOW_QUALITY
    return normalWhiteoutBlend(_getTexNormal(uv), _getSplatNormal(uv));
#endif //LOW_QUALITY
}
#endif //PRE_PASS

#endif //_TERRAIN_SPLATTING_FRAG_