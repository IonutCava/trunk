
void applyShadowPoint(in int shadowIndex, inout float shadow) {
	// SHADOW MAPS
    Light currentLight = dvd_LightSource[dvd_lightIndex[shadowIndex]];
    vec4 position_ls = currentLight._position;
	vec4 abs_position = abs(position_ls);
	float fs_z = -max(abs_position.x, max(abs_position.y, abs_position.z));
    vec4 clip = (currentLight._lightVP0 * _vertexWV) * vec4(0.0, 0.0, fs_z, 1.0);
	float depth = (clip.z / clip.w) * 0.5 + 0.5;
	shadow = texture(texDepthMapFromLightCube, vec4(position_ls.xyz, depth));
}