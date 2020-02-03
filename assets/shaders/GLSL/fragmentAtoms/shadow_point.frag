#ifndef _SHADOW_POINT_FRAG_
#define _SHADOW_POINT_FRAG_

float getShadowFactorPoint(in uint idx, in vec4 details) {
    // SHADOW MAPS
    vec3 position_ls = dvd_shadowLightPosition[idx * 6].xyz;
    vec3 abs_position = abs(position_ls);
    float fs_z = -max(abs_position.x, max(abs_position.y, abs_position.z));
    vec4 clip = (dvd_shadowLightVP[idx * 6] * VAR._vertexW) * vec4(0.0, 0.0, fs_z, 1.0);
    float depth = (clip.z / clip.w) * 0.5 + 0.5;
    return texture(texDepthMapFromLightCube, vec4(position_ls.xyz, details.y), depth).r;
}

#endif //_SHADOW_POINT_FRAG_