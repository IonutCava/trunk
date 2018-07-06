uniform float time;
uniform float windDirectionX;
uniform float windDirectionZ;
uniform float windSpeed;
				
void main(void)
{
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	vec4 vertex = gl_Vertex;
	vec4 vertexMV = gl_ModelViewMatrix * gl_Vertex;
	vec3 normalMV = gl_NormalMatrix * gl_Normal;
	
	vec4 vertexM = gl_TextureMatrix[0] * gl_ModelViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	
	// Animarea copacilor
	float move_speed = (float(int(vertexM.y*vertexM.z) % 50)/85.0 + 0.5);
     move_speed *= windSpeed;
	
	if(windDirectionX == 1 || windDirectionX == -1)
     {
           float move_offset = vertexM.x;
		vertex.x += (0.008 * pow(vertex.y, 2.0) * cos(time * move_speed + move_offset))*windDirectionX;
     }
     if(windDirectionZ == 1 || windDirectionZ == -1)
     {
           float move_offset = vertexM.z;
		vertex.z += (0.008 * pow(vertex.y, 2.0) * cos(time * move_speed + move_offset))*windDirectionZ;
     }
	
	vec4 vLightPosMV = -gl_LightSource[0].position;		
	float intensity = dot(vLightPosMV.xyz, normalMV);
	gl_FrontColor = vec4(intensity, intensity, intensity, 1.0);
	gl_FrontColor.a = 1.0 - clamp(length(vertexMV)/120.0, 0.0, 1.0);
		
	gl_Position = gl_ModelViewProjectionMatrix * vertex;

}






