uniform float time;
uniform float lod_metric;
//uniform float scale;
uniform float windDirectionX;
uniform float windDirectionZ;
uniform float windSpeed;

void main(void)
{
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	vec4 vertex = gl_Vertex;
	vec4 vertexMV = gl_ModelViewMatrix * gl_Vertex;
	vec3 normalMV = gl_NormalMatrix * gl_Normal;
	

	if(gl_Normal.y < 0.0 ) {
		normalMV = -normalMV;
		vertex.x += (0.5*cos(time*windSpeed) * cos(vertex.x) * sin(vertex.x))*windDirectionX;
		vertex.z += (0.5*sin(time*windSpeed) * cos(vertex.x) * sin(vertex.x))*windDirectionZ;
		//vertex.x *= scale;
		//vertex.z *= scale;
		
	}
	
	vec4 vLightPosMV = -gl_LightSource[0].position;	
	float intensity = dot(vLightPosMV.xyz, normalMV);
	gl_FrontColor = vec4(intensity, intensity, intensity, 1.0);
	gl_FrontColor.a = 1.0 - clamp(length(vertexMV)/lod_metric, 0.0, 1.0);
		
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	
	gl_TexCoord[1] = gl_TextureMatrix[0] * gl_Vertex;		
}





