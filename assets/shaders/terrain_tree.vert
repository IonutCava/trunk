uniform float time;
uniform vec3  scale;
uniform float windDirectionX;
uniform float windDirectionZ;
uniform float windSpeed;
				
void main(void)
{
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	vec4 vertex = gl_Vertex;
	vec4 vertexMV = gl_ModelViewMatrix * vertex;
	vec3 normalMV = normalize(gl_NormalMatrix * gl_Normal);
	
	vec4 vertexM = gl_TextureMatrix[0] * gl_ModelViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	
	float move_speed = (float(int(vertexM.y*vertexM.z) % 50)/50.0 + 0.5);
	float move_offset = vertexM.x;

	vertex.x += 0.003 * pow(vertex.y, 2.0) * scale.y * cos(time + move_offset);
	
	
	vec4 vLightPosMV = -gl_LightSource[0].position;
	float intensity = dot(vLightPosMV.xyz, normalMV);
	gl_FrontColor = vec4(intensity, intensity, intensity, 1.0);
	gl_FrontColor.a = 1.0 - clamp(length(vertexMV)/120.0, 0.0, 1.0);
		
		
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
}












