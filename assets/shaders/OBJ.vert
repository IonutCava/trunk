
void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;

	vec4 vertex = gl_Vertex;
	vec4 vertexMV = gl_ModelViewMatrix * gl_Vertex;
	vec3 normalMV = normalize(gl_NormalMatrix * gl_Normal);
	
	vec4 vertexM = gl_TextureMatrix[0] * gl_ModelViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	
	vec4 vLightPosMV = -gl_LightSource[0].position;		
	float intensity = dot(vLightPosMV.xyz, normalMV);
	gl_FrontColor = vec4(intensity, intensity, intensity, 1.0);
	gl_FrontColor.a = 1.0 - clamp(length(vertexMV)/120.0, 0.0, 1.0);
		
	gl_Position = gl_ModelViewProjectionMatrix * vertex;

}







