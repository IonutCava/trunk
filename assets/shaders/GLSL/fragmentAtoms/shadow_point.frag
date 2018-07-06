
float applyShadowPoint(const in uint lightIndex, const in Shadow currentShadowSource) {
	// SHADOW MAPS
    vec3 position_ls = currentShadowSource._lightPosition[0].xyz;
	vec3 abs_position = abs(position_ls);
	float fs_z = -max(abs_position.x, max(abs_position.y, abs_position.z));
    vec4 clip = (currentShadowSource._lightVP0 * _vertexW) * vec4(0.0, 0.0, fs_z, 1.0);
	float depth = (clip.z / clip.w) * 0.5 + 0.5;
    return texture(texDepthMapFromLightCube[lightIndex], vec4(position_ls.xyz, depth));
}