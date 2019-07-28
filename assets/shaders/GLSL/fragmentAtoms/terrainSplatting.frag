#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

//#define TRI_PLANAR_BLEND

layout(binding = TEXTURE_TERRAIN_SPLAT) uniform sampler2DArray texBlendMaps;
layout(binding = TEXTURE_UNIT1)  uniform sampler2DArray helperTextures;

#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
layout(binding = TEXTURE_NORMALMAP) uniform sampler2DArray texNormalMaps;
#endif

#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
layout(binding = TEXTURE_SPECULAR) uniform sampler2D texNormals;
#endif

#if !defined(PRE_PASS)
layout(binding = TEXTURE_UNIT0)  uniform sampler2D texAlbedo;
layout(binding = TEXTURE_TERRAIN_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
#endif

#include "texturing.frag"

const int tiling[] = {
    //int(TEXTURE_TILE_SIZE * 1.0f),
    int(TEXTURE_TILE_SIZE * 0.25f),
    int(TEXTURE_TILE_SIZE * 0.0625f),
    int(TEXTURE_TILE_SIZE * 0.015625f),
    int(TEXTURE_TILE_SIZE * 0.00390625f),
    int(TEXTURE_TILE_SIZE * 0.0009765625f),
};

vec4 _getTexture(in sampler2DArray tex, in vec3 coords) {
#if 1
    if (LoD == 0) {
        return vec4(textureNoTile(tex, helperTextures, 3, coords), 1.0f);
    }
     
    return vec4(textureNoTile(tex, coords), 1.0f); 
#else
    return texture(tex, coords);
#endif
}

#if !defined(PRE_PASS)
vec4 getTerrainAlbedo(in vec2 uv) {
#if defined(LOW_QUALITY)
    return texture(texAlbedo, uv);
#else
    if (LoD > 1) {
        return texture(texAlbedo, uv);
    }

    const int tileScale = tiling[LoD];

    vec4 colour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float detail = 1.0f;

    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const vec4 blendColour = texture(texBlendMaps, vec3(uv, i));

        const uint layerCount = CURRENT_LAYER_COUNT[i];
        for (uint j = 0; j < layerCount; ++j) {
            float amnt = blendColour[j];
            const uint aSlice = ALBEDO_IDX[offset + j];
            if (aSlice < 255) {
                vec3 coordsC = vec3(scaledTextureCoords(uv, ALBEDO_TILING * tileScale), aSlice);
                colour = mix(colour, _getTexture(texTileMaps, coordsC), amnt);
            }
            const uint dSlice = DETAIL_IDX[offset + j];
            if (dSlice < 255) {
                vec3 coordsD = vec3(scaledTextureCoords(uv, (DETAIL_TILING * tileScale)), dSlice);
                detail = mix(detail, _getTexture(texTileMaps, coordsD).r, amnt);
            }
        }

        offset += CURRENT_LAYER_COUNT[i];
    }

    float linearDepth = ToLinearDepth(getDepthValue(dvd_screenPositionNormalised));
    return mix(colour * detail * DETAIL_BRIGHTNESS, colour, saturate(linearDepth * 0.1f));
#endif
}
#endif //PRE_PASS

#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
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
    const float tileScale = ALBEDO_TILING * tiling[LoD];

    vec3 tbn = vec3(1.0f);
    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const uint layerCount = CURRENT_LAYER_COUNT[i];

        vec4 blendColour = texture(texBlendMaps, vec3(uv, i));
        for (uint j = 0; j < layerCount; ++j) {
            uint nSlice = NORMALS_IDX[offset + j];
            if (nSlice == 255) {
                continue;
            }

            vec3 scaledCoords = vec3(scaledTextureCoords(uv, tileScale), nSlice);
            tbn = mix(tbn, _getTexture(texNormalMaps, scaledCoords).rgb, blendColour[j]);
        }
        offset += CURRENT_LAYER_COUNT[i];
    }

    vec3 ret = normalize(2.0f * tbn - 1.0f);

    //const vec3 V = normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);
    //const vec3 normalW = inverse(mat3(dvd_ViewMatrix)) * VAR._normalWV;
    //return perturb_normal(ret, normalW, V, uv);
    return ret;
}

vec3 TerrainNormal(in vec2 uv, in float crtDepth) {
#if defined(LOW_QUALITY)
    return  VAR._normalWV;
#else //LOW_QUALITY
    float distance = saturate(ToLinearDepth(crtDepth) * 0.05f);
    return mix(VAR._tbn * _getSplatNormal(uv), VAR._normalWV, distance);
#endif //LOW_QUALITY
}
#endif //PRE_PASS || !USE_DEFERRED_NORMALS

#endif //_TERRAIN_SPLATTING_FRAG_